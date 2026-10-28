#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "share/schema/ob_column_schema.h"

namespace oceanbase {
namespace share {
namespace schema {

TEST(AIColumnSchema, definition_survives_copy_reset_and_serialization)
{
  ObColumnSchemaV2 source;
  source.set_column_id(18);
  source.set_table_id(500001);
  source.set_data_type(common::ObJsonType);
  source.set_ai_max_retries(7);
  std::string expression = "AI_COMPLETE('m', CONCAT(content, 'Return exactly one valid JSON value'))";
  ASSERT_EQ(0, source.set_ai_column_expr(common::ObString(expression.c_str())));
  expression.assign(expression.size(), 'x');
  ASSERT_TRUE(source.is_ai_column());
  EXPECT_TRUE(source.get_cur_default_value().is_null());
  EXPECT_TRUE(source.get_orig_default_value().is_null());
  ObColumnSchemaV2 copied;
  ASSERT_EQ(0, copied.assign(source));
  source.reset();
  EXPECT_FALSE(source.is_ai_column());
  EXPECT_EQ(2, source.get_ai_max_retries());
  ASSERT_TRUE(copied.is_ai_column());
  EXPECT_EQ(7, copied.get_ai_max_retries());
  EXPECT_EQ(0, copied.get_ai_column_expr().compare(
      "AI_COMPLETE('m', CONCAT(content, 'Return exactly one valid JSON value'))"));
  std::vector<char> bytes(copied.get_serialize_size());
  int64_t position = 0;
  ASSERT_EQ(0, copied.serialize(bytes.data(), bytes.size(), position));
  EXPECT_EQ(bytes.size(), static_cast<size_t>(position));
  ObColumnSchemaV2 restored;
  position = 0;
  ASSERT_EQ(0, restored.deserialize(bytes.data(), bytes.size(), position));
  EXPECT_TRUE(restored.is_ai_column());
  EXPECT_EQ(0, restored.get_ai_column_expr().compare(copied.get_ai_column_expr()));
  EXPECT_EQ(7, restored.get_ai_max_retries());
  EXPECT_TRUE(restored.get_cur_default_value().is_null());
  EXPECT_TRUE(restored.get_orig_default_value().is_null());
  EXPECT_FALSE(restored.is_generated_column());
  EXPECT_EQ(common::ObJsonType, restored.get_data_type());
}

TEST(AIColumnSchema, ordinary_schema_round_trip_has_no_ai_definition)
{
  ObColumnSchemaV2 ordinary;
  std::vector<char> bytes(ordinary.get_serialize_size());
  int64_t position = 0;
  ASSERT_EQ(0, ordinary.serialize(bytes.data(), bytes.size(), position));
  ObColumnSchemaV2 restored;
  ASSERT_EQ(0, restored.set_ai_column_expr(common::ObString("AI_COMPLETE('m', content)")));
  restored.set_ai_max_retries(99);
  position = 0;
  ASSERT_EQ(0, restored.deserialize(bytes.data(), bytes.size(), position));
  EXPECT_FALSE(restored.is_ai_column());
  EXPECT_EQ(2, restored.get_ai_max_retries());
}

} // namespace schema
} // namespace share
} // namespace oceanbase
