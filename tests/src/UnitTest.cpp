#include <gtest/gtest.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include "./../../include/SimpleJson.hpp"

string validExample = "{\"person\": {\"name\": \"charlie\", \"skills\": true, \"age\": 27}}";
string validExampleBasic = "{\"name\": \"charlie\", \"skills\": true, \"age\": 27}";
string basicSetString = "{\"name\": \"charlie\", \"skills\": \"coding\", \"age\": 27}";
string basicSetBool = "{\"name\": \"charlie\", \"skills\": true, \"age\": 27}";
string basicSetFloat = "{\"name\": \"charlie\", \"skills\": 56.000000, \"age\": 27}";
string validArrayExample = "{\"name\": \"charlie\", \"skills\": [5, \"drawing\", false], \"drives\": \"yes\"}";
string validArrayExampleBasic = "[5, \"drawing\", false]";
string basicArraySetString = "[5, \"coding\", false]";
string invalidExample = "{\"person\": {\"name\": \"charlie\", \"skills\": true, \"age\": 27}}}";

void removeWhitespace(string& str) {
	str.erase(remove(str.begin(), str.end(), ' '), str.end());
	str.erase(remove(str.begin(), str.end(), '\n'), str.end());
	str.erase(remove(str.begin(), str.end(), '\t'), str.end());
}

TEST(constructor, succeedsIfValidJsonString) {
	try {
		SimpleJson testJson = SimpleJson(validExample);
		SUCCEED();
	} catch (int err) {
		FAIL();
	}
}

TEST(constructor, throwsIfInvalidJsonString) {
	EXPECT_THROW({
		SimpleJson testJson = SimpleJson(invalidExample);
	}, invalid_argument);
}

TEST(constructor, succeedsIfValidJsonFile) {
	try {
		ifstream stream("./../examples/small-valid.json");
		SimpleJson fileJson(stream);
		stream.close();
		SUCCEED();
	} catch (int err) {
		FAIL();
	}
}

TEST(constructor, throwsIfInvalidJsonSmall) {
	EXPECT_THROW({
		ifstream stream("./../examples/small-invalid.json");
		SimpleJson fileJson(stream);
		stream.close();
	}, invalid_argument);
}

TEST(constructor, throwsIfInvalidJsonMedium) {
	EXPECT_THROW({
		ifstream stream("./../examples/medium-invalid.json");
		SimpleJson fileJson(stream);
		stream.close();
	}, invalid_argument);
}

TEST(serialization, serializeOutputCorrectSmall) {
	SimpleJson testJson = SimpleJson(validExample);
	string input = validExample;
	removeWhitespace(input);
	string output = testJson.serialize();
	removeWhitespace(output);
	EXPECT_EQ(input, output);
}

TEST(serialization, serializeOutputCorrectMedium) {
	ifstream inputStream("./../examples/medium-valid.json");
	SimpleJson fileJson(inputStream);
	ifstream compareStream("./../examples/medium-valid.json");
	std::stringstream buffer;
	buffer << compareStream.rdbuf();
	string input = buffer.str();
	removeWhitespace(input);
	string output = fileJson.serialize();
	removeWhitespace(output);
	EXPECT_EQ(input, output);
}

TEST(serialization, serializeOutputCorrectLarge) {
	ifstream inputStream("./../examples/large-valid.json");
	SimpleJson fileJson(inputStream);
	ifstream compareStream("./../examples/large-valid.json");
	std::stringstream buffer;
	buffer << compareStream.rdbuf();
	string input = buffer.str();
	removeWhitespace(input);
	string output = fileJson.serialize();
	removeWhitespace(output);
	EXPECT_EQ(input, output);
}

TEST(serialization, serializeOutputCorrectLarge2) {
	ifstream inputStream("./../examples/large-valid2.json");
	SimpleJson fileJson(inputStream);
	ifstream compareStream("./../examples/large-valid2.json");
	std::stringstream buffer;
	buffer << compareStream.rdbuf();
	string input = buffer.str();
	removeWhitespace(input);
	string output = fileJson.serialize();
	removeWhitespace(output);
	EXPECT_EQ(input, output);
}

TEST(get, getObjectByKey) {
	string input = validExampleBasic;
	removeWhitespace(input);
	SimpleJson testJson = SimpleJson(validExample);
	SimpleJson person = testJson.get("person");
	string output = person.serialize();
	removeWhitespace(output);
	EXPECT_EQ(input, output);
}

TEST(get, getObjectByIndex) {
	string input = validArrayExampleBasic;
	removeWhitespace(input);
	SimpleJson testJson = SimpleJson(validArrayExample);
	SimpleJson skills = testJson.get("skills");	//skills is an array
	string output = skills.serialize();
	removeWhitespace(output);
	EXPECT_EQ(input, output);
}

TEST(get, isStringTrue) {
	SimpleJson testJson = SimpleJson(validExample);
	SimpleJson person = testJson.get("person");
	bool isString;
	isString = person.get("name").isString();
	EXPECT_EQ(true, isString);
}

TEST(get, isStringFalse) {
	SimpleJson testJson = SimpleJson(validExample);
	SimpleJson person = testJson.get("person");
	bool isString;
	isString = person.get("age").isString();
	EXPECT_EQ(false, isString);
}

TEST(get, isBoolTrue) {
	SimpleJson testJson = SimpleJson(validExample);
	SimpleJson person = testJson.get("person");
	bool isBool;
	isBool = person.get("skills").isBool();
	EXPECT_EQ(true, isBool);
}

TEST(get, isBoolFalse) {
	SimpleJson testJson = SimpleJson(validExample);
	SimpleJson person = testJson.get("person");
	bool isBool;
	isBool = person.get("name").isBool();
	EXPECT_EQ(false, isBool);
}

TEST(get, isFloatTrue) {
	SimpleJson testJson = SimpleJson(validExample);
	SimpleJson person = testJson.get("person");
	bool isFloat;
	isFloat = person.get("age").isFloat();
	EXPECT_EQ(true, isFloat);
}

TEST(get, isFloatFalse) {
	SimpleJson testJson = SimpleJson(validExample);
	SimpleJson person = testJson.get("person");
	bool isFloat;
	isFloat = person.get("skills").isFloat();
	EXPECT_EQ(false, isFloat);
}

TEST(get, getStringValid) {
	SimpleJson testJson = SimpleJson(validExample);
	SimpleJson person = testJson.get("person");
	string name;
	name = person.get("name").getString();
	EXPECT_EQ("charlie", name);
}

TEST(get, getStringInvalid) {
	EXPECT_THROW({
		SimpleJson testJson = SimpleJson(validExample);
		SimpleJson person = testJson.get("person");
		string skills;
		skills = person.get("skills").getString();	//skills is a bool
	}, invalid_argument);
}

TEST(get, getStringByIndex) {
	SimpleJson testJson = SimpleJson(validArrayExampleBasic);
	string skills;
	skills = testJson.get(1).getString();
	EXPECT_EQ("drawing", skills);
}

TEST(get, getBoolValid) {
	SimpleJson testJson = SimpleJson(validExample);
	SimpleJson person = testJson.get("person");
	bool skills;
	skills = person.get("skills").getBool();
	EXPECT_EQ(true, skills);
}

TEST(get, getNumberValid) {
	SimpleJson testJson = SimpleJson(validExample);
	SimpleJson person = testJson.get("person");
	float age;
	age = person.get("age").getFloat();
	EXPECT_EQ(27, age);
}

TEST(set, setStringByKey) {
	SimpleJson testJson = SimpleJson(validExampleBasic);
	testJson.key("skills").setString("coding");
	string output = testJson.get("skills").getString();
	EXPECT_EQ("coding", output);
}

TEST(set, setStringByIndex) {
	SimpleJson testJson = SimpleJson(validArrayExampleBasic);
	testJson.key(1).setString("coding");
	string output = testJson.get(1).getString();
	EXPECT_EQ("coding", output);
}

TEST(set, setBoolByKey) {
	SimpleJson testJson = SimpleJson(validExampleBasic);
	testJson.key("skills").setBool(true);
	bool output = testJson.get("skills").getBool();
	EXPECT_EQ(true, output);
}

TEST(set, setBoolByIndex) {
	SimpleJson testJson = SimpleJson(validArrayExampleBasic);
	testJson.key(1).setBool(false);
	bool output = testJson.get(1).getBool();
	EXPECT_EQ(false, output);
}

TEST(set, setFloatByKey) {
	SimpleJson testJson = SimpleJson(validExampleBasic);
	testJson.key("skills").setFloat(56);
	float output = testJson.get("skills").getFloat();
	EXPECT_EQ(56, output);
}

TEST(set, setFloatByIndex) {
	SimpleJson testJson = SimpleJson(validArrayExampleBasic);
	testJson.key(1).setFloat(56);
	float output = testJson.get(1).getFloat();
	EXPECT_EQ(56, output);
}

int main(int argc, char** argv) {
	::testing::InitGoogleTest(&argc, argv);
	RUN_ALL_TESTS();
	return 0;
}