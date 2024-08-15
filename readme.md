# SIMPLEJSON
### A lightweight json library for C++ with a simple user interface
- SimpleJson uses a DOM-style tree data structure to store JSON elements.
- The user API aims to be simple, combining modern JSON syntax with a cpp-friendly style. 
### Installation

SimpleJson can be installed through a single include file found in include/SimpleJson.hpp

### Usage
**Deserialize a json string literal**
```
string jsonString = "{
	\"person\": {
		\"name\": \"bob\",
		\"age\": 45,
		\"skills\": [
			\"frontend\",
			\"backend\",
			\"cloud\"
		]
	},
	\"remote\": false
}"
SimpleJson myJson(jsonString);
```
**Deserialize a json file**
```
ifstream stream("./examples/bob.json");
SimpleJson myJson(stream);
stream.close();
```

**Serialize (stringify) a json object**
```
std::cout << myJson.serialize() << endl;

{
	"person": {
		"name": "bob",
		"age": 45,
		"skills": [
			"frontend",
			"backend",
			"cloud"
		]
	},
	"remote": false
}
```
---

**Get a json object by key**
```
SimpleJson skills = myJson.get("person").get("skills");
```
(this would be equivalent to `SimpleJson skills = myJson["person"]["skills"]`)

**Get a json object by index**
```
SimpleJson firstSkill = skills.get(0);
```

**Get a string value from a json object**
```
std::string skill;
if (firstSkill.isString())
{
	skill = firstSkill.getString();
} 
```

**Get a bool value from a json object**
```
SimpleJson remote = myJson.get("person").get("remote");

bool isRemote;
if (remote.isBool())
{
	isRemote = remote.getBool();
} 
```

**Get a float value from a json object**
```
SimpleJson age = myJson.get("person").get("age");

float steveAge;
if (age.isFloat())
{
	steveAge = age.getFloat();
} 
```
---

**Set a json value to string by key**
```
myJson.key("person").key("name").set("steve");
```
(this would be equivalent to `SimpleJson myJson["person"]["name"] = "steve"`)

**Set a json value to bool by key**
```
myJson.key("person").key("remote").set(false);
```
**Set a json value to float by key**
```
myJson.key("person").key("age").set("51");
```
**Set a json value to string by index**
```
myJson.key("person").key("skills").key(2).set("testing");
```
**Set a new json value to string by key**
```
myJson.key("person").key("city").set("london");
```

