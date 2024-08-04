#include "SimpleJson.hpp"

int main() {
string example = "{\"person\": {\"name\": \"charlie\", \"skills\": true, \"age\": 27}}";
string arrayExample = "{\"name\": \"charlie\", \"skills\": [5, \"drawing\", false], \"drives\": \"yes\"}";
string largeExample = "{ \"name\":\"charlie\", \"age\":24, \"parents\": { \"mother\": true, \"father\": { \"name\": \"steve\", \"age\": \"50\" }}, \"dob\": \"123456\"}";

//read json from file
ifstream stream("./json-examples/test.json");
SimpleJson fileJson(stream);
stream.close();

//save json to file
ofstream ostream("./json-examples/test-out.json");
ostream << fileJson.generateJsonString();
ostream.close();

//read json from string literal
SimpleJson exampleJson(example);

//compare input string literal to serialised json object
cout << exampleJson.m_jsonString << endl;
cout << exampleJson.generateJsonString() << endl;

//get json object
SimpleJson person = exampleJson.get("person");

//serialise the new json
cout << person.generateJsonString() << endl;

//get string value from json
string name;
if (person.get("name").isString()) {
	name = person.get("name").getString();
}

//get bool value from json
bool skills;
if (person.get("skills").isBool()) {
	skills = person.get("skills").getBool();
}

//get float value from json
float age;
if (person.get("age").isFloat()) {
	age = person.get("age").getFloat();
}

//serialise the modified json
cout << person.generateJsonString() << endl;

//set json value to string
person.key("skills").setString("coding");

//set json value to bool
person.key("skills").setBool(false);

//set json value to float
person.key("age").setFloat(36.5);

//set new json value
person.key("city").setString("london");

//serialise the modified json
cout << person.generateJsonString() << endl;

//read json with array from string literal
SimpleJson individual(arrayExample);

//get json array
SimpleJson my_skills = individual.get("skills");

//serialise new json
cout << my_skills.generateJsonString() << endl;

//get string value from array
string skill;
if (my_skills.get(1).isString()) {
	skill = my_skills.get(1).getString();
}

//set float value to array
my_skills.key(2).setFloat(567);

//add a string to array (null values are added inbewteen)
my_skills.key(6).setString("dancing");

//serialise the modified json
cout << my_skills.generateJsonString() << endl;

}