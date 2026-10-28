#include <gtest/gtest.h>
#include <string>
#include "sql/parser/ob_parser.h"
#include "common/sql_mode/ob_sql_mode.h"
#include "lib/allocator/ob_allocator.h"

namespace oceanbase {
namespace sql {
namespace {
const ParseNode *find_ai_column(const ParseNode *node)
{
  if (node == nullptr || node->type_ == T_AI_COLUMN) {
    return node;
  }
  for (int64_t i = 0; i < node->num_child_; ++i) {
    const ParseNode *found = find_ai_column(node->children_[i]);
    if (found != nullptr) {
      return found;
    }
  }
  return nullptr;
}

TEST(AIColumnParser, create_preserves_prompt_span_and_decoded_model)
{
  common::ObArenaAllocator allocator;
  ObParser parser(allocator, DEFAULT_MYSQL_MODE);
  ParseResult result = {};
  const char *sql = "CREATE TABLE docs(id BIGINT PRIMARY KEY, content VARCHAR(200), "
      "tags JSON AI COLUMN (AI_COMPLETE('text_model', CONCAT('a,b: ', content))) "
      "WITH (sync_mode='async', max_retries=2))";
  ASSERT_EQ(0, parser.parse(common::ObString(sql), result));
  const ParseNode *ai = find_ai_column(result.result_tree_);
  ASSERT_NE(nullptr, ai);
  ASSERT_EQ(2, ai->num_child_);
  const ParseNode *expr = ai->children_[0];
  ASSERT_NE(nullptr, expr);
  ASSERT_EQ(T_FUN_SYS, expr->type_);
  const ParseNode *args = expr->children_[1];
  ASSERT_EQ(2, args->num_child_);
  const ParseNode *model = args->children_[0];
  EXPECT_EQ(T_VARCHAR, model->type_);
  EXPECT_EQ("text_model", std::string(model->str_value_, model->str_len_));
  const ParseNode *prompt = args->children_[1];
  const int64_t offset = prompt->stmt_loc_.first_column_ - expr->stmt_loc_.first_column_;
  const int64_t length = prompt->stmt_loc_.last_column_ - prompt->stmt_loc_.first_column_ + 1;
  ASSERT_GE(offset, 0);
  ASSERT_LE(offset + length, expr->str_len_);
  EXPECT_EQ("CONCAT('a,b: ', content)", std::string(expr->str_value_ + offset, length));
  ASSERT_NE(nullptr, ai->children_[1]);
  EXPECT_EQ(2, ai->children_[1]->num_child_);
  parser.free_result(result);
}

TEST(AIColumnParser, alter_add_modify_and_ordinary_ai_identifier)
{
  const char *statements[] = {
    "ALTER TABLE docs ADD emb VECTOR(4) AI COLUMN (AI_EMBED('embed_model', content))",
    "ALTER TABLE docs MODIFY tags JSON AI COLUMN (AI_COMPLETE('text_model', content)) WITH (max_retries=0)",
    "CREATE TABLE ordinary(ai INT, sync_mode VARCHAR(10), max_retries INT)",
    "SHOW AI COLUMNS FROM docs",
    "SHOW AI COLUMNS FROM db.docs WHERE Field='tags'",
    "SHOW AI COLUMNS FROM docs FROM db LIKE 'ta%'",
    "ALTER TABLE docs RETRY AI COLUMN tags WHERE id=1, ADD COLUMN extra_col INT",
    "CREATE TABLE docs(id INT, tags JSON AI COLUMN (AI_COMPLETE('text_model', JSON_OBJECT('text', 'x,y'))))"
  };
  for (const char *sql : statements) {
    common::ObArenaAllocator allocator;
    ObParser parser(allocator, DEFAULT_MYSQL_MODE);
    ParseResult result = {};
    EXPECT_EQ(0, parser.parse(common::ObString(sql), result)) << sql;
    parser.free_result(result);
  }
}

TEST(AIColumnParser, retry_lowers_to_native_update)
{
  const char *statements[] = {
    "ALTER TABLE docs RETRY AI COLUMN tags",
    "ALTER TABLE docs RETRY AI COLUMN tags WHERE id IN (1, 3)",
    "ALTER TABLE docs RETRY AI COLUMN tags WHERE status='FAILED'",
    "ALTER TABLE docs RETRY AI COLUMN tags WHERE `status`='active'"
  };
  for (const char *sql : statements) {
    common::ObArenaAllocator allocator;
    ObParser parser(allocator, DEFAULT_MYSQL_MODE);
    ParseResult result = {};
    ASSERT_EQ(0, parser.parse(common::ObString(sql), result)) << sql;
    ASSERT_NE(nullptr, result.result_tree_);
    ASSERT_EQ(T_STMT_LIST, result.result_tree_->type_);
    ASSERT_EQ(1, result.result_tree_->num_child_);
    const ParseNode *update = result.result_tree_->children_[0];
    ASSERT_NE(nullptr, update);
    EXPECT_EQ(T_UPDATE, update->type_);
    EXPECT_EQ(0, update->value_);
    ASSERT_EQ(11, update->num_child_);
    const ParseNode *assignments = update->children_[2];
    ASSERT_NE(nullptr, assignments);
    ASSERT_EQ(T_ASSIGN_LIST, assignments->type_);
    ASSERT_EQ(1, assignments->num_child_);
    ASSERT_NE(nullptr, assignments->children_[0]);
    EXPECT_EQ(0x41494352, assignments->children_[0]->value_);
    parser.free_result(result);
  }
}

TEST(AIColumnParser, rejects_unbalanced_options)
{
  common::ObArenaAllocator allocator;
  ObParser parser(allocator, DEFAULT_MYSQL_MODE);
  ParseResult result = {};
  EXPECT_NE(0, parser.parse(common::ObString(
      "CREATE TABLE docs(x JSON AI COLUMN (AI_COMPLETE('m', 'x')) WITH (max_retries=2,) )"), result));
  parser.free_result(result);
}
} // namespace
} // namespace sql
} // namespace oceanbase
