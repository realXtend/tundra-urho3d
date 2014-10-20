
#include "TestRunner.h"
#include "TestBenchmark.h"

#include "JSON/JSON.h"

using namespace Tundra;
using namespace Tundra::Test;

const String StringData = "\"  Json String Test\tHello World  \"";
const String ObjectData = "{\n\"Hello\":true  ,   \"World\"  \n:  00010, \"Str\" :" + StringData + " , \"Arr\" : [1,2],\"Obj\":{\"test\" : true}  }";
const String ArrayData  = "[ \"Hello World\", 0  ,-1234,   1234.5678  ,  true,false,   \n   null, [ \"World Hello\", 10, false ]  \t , " + ObjectData + "  \n]";

TEST_F(Runner, ParseJSON)
{
    Tundra::Benchmark::Iterations = 10000;

    BENCHMARK("Null", 10)
    {
        JSONValue value;

        ASSERT_TRUE(value.FromString(" \t \n     null   "));
        BENCHMARK_STEP_END;

        ASSERT_EQ(value.Type(), JSON_NULL);
        ASSERT_TRUE(value.IsNull());
    }
    BENCHMARK_END;

    Tundra::Benchmark::Iterations = 10000;

    BENCHMARK("Bool", 10)
    {
        JSONValue value;

        ASSERT_TRUE(value.FromString(" \t \n     true   "));
        BENCHMARK_STEP_END;

        ASSERT_EQ(value.Type(), JSON_BOOL);
        ASSERT_TRUE(value.IsBool());
        ASSERT_TRUE(value.GetBool());
    }
    BENCHMARK_END;

    Tundra::Benchmark::Iterations = 10000;

    BENCHMARK("Number", 10)
    {
        JSONValue value;

        ASSERT_TRUE(value.FromString("  \n    1238972313.12312412   "));
        BENCHMARK_STEP_END;

        ASSERT_EQ(value.Type(), JSON_NUMBER);
        ASSERT_TRUE(value.IsNumber());
        ASSERT_DOUBLE_EQ(value.GetNumber(), 1238972313.12312412);
    }
    BENCHMARK_END;

    Tundra::Benchmark::Iterations = 10000;

    BENCHMARK("String", 10)
    {
        JSONValue value;

        ASSERT_TRUE(value.FromString(StringData));
        BENCHMARK_STEP_END;

        ASSERT_EQ(value.Type(), JSON_STRING);
        ASSERT_TRUE(value.IsString());

        ASSERT_EQ(value.GetString(), "  Json String Test\tHello World  ");
    }
    BENCHMARK_END;

    Tundra::Benchmark::Iterations = 10000;

    BENCHMARK("Array", 10)
    {
        JSONValue value;

        ASSERT_TRUE(value.FromString(ArrayData));
        BENCHMARK_STEP_END;

        ASSERT_EQ(value.Type(), JSON_ARRAY);
        ASSERT_TRUE(value.IsArray());

        ASSERT_EQ(value[0].GetString(), "Hello World");
        ASSERT_DOUBLE_EQ(value[1].GetNumber(), 0.0);
        ASSERT_DOUBLE_EQ(value[2].GetNumber(), -1234.0);
        ASSERT_DOUBLE_EQ(value[3].GetNumber(), 1234.5678);
        ASSERT_TRUE(value[4].GetBool());
        ASSERT_FALSE(value[5].GetBool());
        ASSERT_TRUE(value[6].IsNull());

        // Embedded array
        ASSERT_TRUE(value[7].IsArray());
        ASSERT_TRUE(value[7][0].IsString());
        ASSERT_TRUE(value[7][1].IsNumber());
        ASSERT_TRUE(value[7][2].IsBool());

        // Embedded object
        ASSERT_TRUE(value[8].IsObject());
        JSONObject obj = value[8].GetObject();
        ASSERT_TRUE(obj["Hello"].IsBool() && obj["Hello"].GetBool());
        ASSERT_TRUE(obj["World"].IsNumber());
        ASSERT_DOUBLE_EQ(obj["World"].GetNumber(), 10.0);
        ASSERT_TRUE(obj["Str"].IsString());
        ASSERT_EQ(obj["Str"].GetString().Length(), StringData.Length() - 2);
        // Embedded child array
        ASSERT_TRUE(obj["Arr"].IsArray());
        ASSERT_EQ(obj["Arr"].GetArray()[1], 2);
        // Embedded child Object
        ASSERT_TRUE(obj["Obj"].IsObject());
        JSONObject childObj = obj["Obj"].GetObject();
        ASSERT_TRUE(childObj["test"].GetBool());
    }
    BENCHMARK_END;

    Tundra::Benchmark::Iterations = 10000;

    BENCHMARK("Object", 10)
    {
        JSONValue value;

        ASSERT_TRUE(value.FromString(ObjectData));
        BENCHMARK_STEP_END;

        ASSERT_EQ(value.Type(), JSON_OBJECT);
        ASSERT_TRUE(value.IsObject());

        JSONObject obj = value.GetObject();
        ASSERT_TRUE(obj["Hello"].IsBool() && obj["Hello"].GetBool());
        ASSERT_TRUE(obj["World"].IsNumber());
        ASSERT_DOUBLE_EQ(obj["World"].GetNumber(), 10.0);
        ASSERT_TRUE(obj["Str"].IsString());
        ASSERT_EQ(obj["Str"].GetString().Length(), StringData.Length() - 2);
        // Embedded array
        ASSERT_TRUE(obj["Arr"].IsArray());
        ASSERT_EQ(obj["Arr"].GetArray()[1], 2);
        // Embedded Object
        ASSERT_TRUE(obj["Obj"].IsObject());
        JSONObject childObj = obj["Obj"].GetObject();
        ASSERT_TRUE(childObj["test"].GetBool());
    }
    BENCHMARK_END;
}

TUNDRA_TEST_MAIN();
