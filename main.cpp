#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <algorithm>
#include <iterator>
#include <set>
#include <windows.h>
using namespace std;
/**
 * @class Attribute
 * store invididual attributes of the json object, plus information about their parent attributes, and metadata
*/
class Attribute {
public:

	enum valueType {
		UNKNOWN,
		EMPTY,
		STRING,
		BOOL,
		NUMBER,
		OBJECT,
		ARRAY
	};

	int m_id;
	int m_parentId;
	string m_key;
	string m_value;
	int m_valueType = EMPTY;


	/**
	 * @fn Attribute() 
	 * constructor for Attribute class
	 * 
	*/
	Attribute() {
		m_id = 0;
	}

	/**
	 * @fn saveValue()
	 * @param value the value to be saved
	 * @param type optional param for type of value being saved from enum valueType
	*/
	void saveValue(string value, int type=UNKNOWN) {
		//need to check for non empty string plus UNknown (do I mean empty string plus unknown ?)
		m_value = value;
		if (value == "") {
			m_valueType = type;
		} else if (value[0] == '\"') {
			m_valueType = STRING;
			m_value = value.substr(1,value.size()-2);
		} else if (value == "true" || value == "false" || value == "null") {
			m_valueType = BOOL;
		} else if (isFloat(value)) {
			m_valueType = NUMBER;
		} else {
			throw invalid_argument("invalid json value");
			//value is not a valid json value
		}
	}

	bool isFloat( string value ) {
		istringstream testStream(value);
		float testFloat;
		testStream >> noskipws >> testFloat;
		return testStream.eof() && !testStream.fail(); 
	}
};

/**
 * @class easyJson
 * Master class for handling json objects. 
 * Stores json object as a vector of Attributes
 * Parses json text and stores resulting json object as a vector of attributes
 * Allows value access via [] operator
 * plus more tbc
*/
class easyJson {
public:

	int m_currentId = 1;
	int m_currentParent = 0;
	int m_baseId = 1;
	int m_setId = -1;
	string m_jsonString;
	string m_cleanString;
	vector<Attribute> m_jsonAttributes;
	
	/**
	 * @fn easyJson constructor
	 * @param jsonAttributes vector of Attributes that represent json object
	 * this constructor is used when creating one easyJson object from another (eg when json is queried)
	*/
	easyJson(vector<Attribute> jsonAttributes) {
		m_jsonAttributes = jsonAttributes;
		m_baseId = m_jsonAttributes[0].m_parentId;	//not sure if this is a really bad idea
		//also prob stringify it and store that into m_jsonString & m_cleanString
	}

	/**
	 * constructor - used when parsing a json string
	 * @param input - string json input to be parsed
	*/
	easyJson(string input) {
		cleanAndParse(input);
	}

	/**
	 * constructor - used when parsing a json file
	 * @param stream - std::ifstream of file to be parsed
	*/
	easyJson(ifstream &stream) {
		string input((istreambuf_iterator<char>(stream)), istreambuf_iterator<char>());
		cleanAndParse(input);
	}

	/**
	 * 
	 * @param input - string json input to be parsed
	*/
	void cleanAndParse(string input) {
		m_jsonString = input;
		m_cleanString = input;
		m_cleanString.erase(remove(m_cleanString.begin(), m_cleanString.end(), ' '), m_cleanString.end());
		m_cleanString.erase(remove(m_cleanString.begin(), m_cleanString.end(), '\n'), m_cleanString.end());
		parseJsonString(m_cleanString);
	}

	/**
	 * @param input {string} - remaining json text to be parsed
	 * finds next special character in json string to denote start / end of current attribute
	 * @return the next special character in the json string
	*/
	char findNextDelimiter(string input) {
		for (char& character : input) {
			switch (character) {
				case ':':
					return ':';
				case ',':
					return ',';
				case '{':
					return '{';
				case '}':
					return '}';
				case '[':
					return '[';
				case ']':
					return ']';
				default:
					continue;
			}
		}

		return '0';
	}

	/**
	 * @param attribute {Attribute} - the attribute to be saved into the attribute vector
	 * this func saves the current attribute into the attribute vector then returns a blank attribute
	 * @return a blank new Attribute to be used for the next step in the json parse
	 * 
	*/
	Attribute saveAttribute (Attribute attribute) {
		attribute.m_id = m_currentId;
		attribute.m_parentId = m_currentParent;
		m_jsonAttributes.push_back(attribute);
		Attribute newAttribute;
		return newAttribute;
	}

	int getAttributeIndex (int id) {
		for (int i = 0; i < m_jsonAttributes.size(); i++) {
			if (m_jsonAttributes[i].m_id == id) {
				return i;
			}
		}
		return -1;
	}

	/**
	 * loop through the json attributes and return the attribute with the specified ID 
	*/
	Attribute getParent (int parentId) {
		Attribute attribute;
		for (int i = 0; i < m_jsonAttributes.size(); i++) {
			Attribute currentAttribute = m_jsonAttributes[i];
			if (currentAttribute.m_id == parentId) {
				attribute = currentAttribute;
			}
		}

		return attribute;
	}

	/**
	 * loop through the attributes and return all which are direct children of specified parent
	*/
	vector<Attribute> getDirectChildren(int parentId) {
		vector<Attribute> childAttributes;
		for (int i = 0; i < m_jsonAttributes.size(); i++) {
			Attribute currentAttribute = m_jsonAttributes[i];
			if (currentAttribute.m_parentId == parentId) {
				childAttributes.push_back(currentAttribute);
			}
		}

		return childAttributes;
	}

	/**
	 * return all attributes which are direct children, or children of those children, of specified parent
	*/
	vector<Attribute> getAllChildren(int parentId) {
		vector<Attribute> childAttributes;
		std::set <int> parentIds = {parentId};
		for (int i = 0; i < m_jsonAttributes.size(); i++) {
			Attribute currentAttribute = m_jsonAttributes[i];
			if (parentIds.find(currentAttribute.m_parentId) != parentIds.end()) {
				childAttributes.push_back(currentAttribute);
				parentIds.insert(currentAttribute.m_id);	//this relies on no children ever appearing before their parent in m_jsonAttributes !!!
			}
		}

		return childAttributes;
	}

	void deleteAllChildren(int parentId) {
		std::set <int> parentIds = {parentId};
		for (int i = 0; i < m_jsonAttributes.size(); i++) {
			Attribute currentAttribute = m_jsonAttributes[i];	//think all these instances of currentAttribute are pointless and waste memory
			if (parentIds.find(currentAttribute.m_parentId) != parentIds.end()) {
				parentIds.insert(currentAttribute.m_id);	//this relies on no children ever appearing before their parent in m_jsonAttributes !!!
				m_jsonAttributes.erase(m_jsonAttributes.begin()+i);
			}
		}
	}

	/**
	 * parse json string, searching for special characters and generating attributes to represent the string as a json object
	 * attribute IDs are used to link parents to children
	*/
	void parseJsonString(string input) {
		bool running = true;
		size_t pos;
		char delimiter;

		Attribute attribute;

		while (running == true) {
			cout << "input: " << input << endl;
			delimiter = findNextDelimiter(input);

			switch (delimiter) {
				case ':':
					pos = input.find_first_of(':');				//put these in functions?
					attribute.m_key = input.substr(1, pos-2);	//strip out "" for keys
					input.erase(0, pos+1);
					break;
				
				case ',':
					pos = input.find_first_of(',');
					attribute.saveValue(input.substr(0, pos), attribute.EMPTY);
					input.erase(0, pos+1);
					attribute = saveAttribute(attribute);
					m_currentId++;
					break;

				case '{':
					pos = input.find_first_of('{');
					attribute.saveValue("", attribute.OBJECT);
					input.erase(0, pos+1);
					attribute = saveAttribute(attribute);
					m_currentParent = m_currentId;
					m_currentId++;
					break;
				
				case '}':
				{
					pos = input.find_first_of('}');
					attribute.saveValue(input.substr(0, pos), attribute.EMPTY); //this is empty string for double closing bracket which gives type: object - but maybe thats fine as its the closing up of the parent object?
					input.erase(0, pos+1);
					attribute = saveAttribute(attribute);
					m_currentId++;
					Attribute parentAttribute = getParent(m_currentParent);
					m_currentParent = parentAttribute.m_parentId;
					break;
				}

				case '[':
				{
					//think array at top level is handled naturally the same way object at top level is - we get an empty attribute that is the parent of everything
					pos = input.find_first_of('[');
					attribute.saveValue("", attribute.ARRAY);
					input.erase(0, pos+1);
					attribute = saveAttribute(attribute);
					m_currentParent = m_currentId;
					m_currentId++;
					break;
				}

				case ']':
				{
					pos = input.find_first_of(']');
					attribute.saveValue(input.substr(0, pos), attribute.EMPTY);
					input.erase(0, pos+1);
					attribute = saveAttribute(attribute);
					m_currentId++;
					Attribute parentAttribute = getParent(m_currentParent);
					m_currentParent = parentAttribute.m_parentId;
					break;
				}

				default:
					cout << "default" << endl;
			}

			if (input == "") {
				running = false;
				if (m_currentParent != 0) {
					throw invalid_argument("not a valid json file");
				}
			}
		}
	}

	string midJson = "{\"name\": \"charlie\", \"skills\": {\"drawing\": true, \"writing\": \"false\"}, \"driver\": \"yes\"}";

	/**
	 * @brief generate string representation of the json attributes - ready to be saved to file
	 */
	string generateJsonString () {
		string output;
		int currentParent = 0;
		bool parentIsArray = false;
		for (int i = 0; i < m_jsonAttributes.size(); i++) {
			Attribute currentAttribute = m_jsonAttributes[i];

			switch (currentAttribute.m_valueType) {
				case currentAttribute.STRING:
					if (!parentIsArray) output.append("\"").append(currentAttribute.m_key).append("\": ");
					output.append("\"").append(currentAttribute.m_value).append("\", ");
					break;
				
				case currentAttribute.BOOL:				
				case currentAttribute.NUMBER:
					if (!parentIsArray) output.append("\"").append(currentAttribute.m_key).append("\": ");
					output.append(currentAttribute.m_value).append(", ");
					break;
				
				case currentAttribute.OBJECT:
					output.append("\"").append(currentAttribute.m_key).append("\": {");
					currentParent = currentAttribute.m_id;
					break;

				case currentAttribute.ARRAY:
					output.append("\"").append(currentAttribute.m_key).append("\": [");
					currentParent = currentAttribute.m_id;
					parentIsArray = true;
					break;
				
				default:
					// this should not be in the default class 
					if (currentAttribute.m_parentId != currentParent) {
						output.erase(output.end()-2, output.end());	//this is ugly but maybe some manual steps are neccessary
						parentIsArray ? output.append("], ") : output.append("}, ");
						currentParent = getParent(currentParent).m_parentId;
						parentIsArray = false;
					}
					break;
			}
		}
		output = output.substr(4, output.size()-6);
		parentIsArray ? output.append("]") : output.append("}");
		
		return output;
	}

	/**
	 * query easyJson object by key
	 * get the attribute which represents the given key
	 * then call getAllChildren to get all its children and return them
	*/
	easyJson get (string key) {
		Attribute attribute;
		//could use getDirectChildren here
		for (int i = 0; i < m_jsonAttributes.size(); i++) {
			Attribute currentAttribute = m_jsonAttributes[i];
			if (currentAttribute.m_key == key && currentAttribute.m_parentId == m_baseId) {
				attribute = currentAttribute;
			}
		}

		vector<Attribute> outputAttributes;
		if (attribute.m_valueType == attribute.OBJECT || attribute.m_valueType == attribute.ARRAY) {
			outputAttributes = getAllChildren(attribute.m_id);
		} else {
			outputAttributes.push_back(attribute);
		}

		return outputAttributes;
	}

	/**
	 * query easyJson object by index (used when object is an array)
	 * gets all direct children of current base attribute and returns them
	*/
	easyJson get (int index) {
		vector<Attribute> attributes = getDirectChildren(m_baseId);
		Attribute attribute = attributes[index];

		vector<Attribute> outputAttributes;
		if (attribute.m_valueType == attribute.OBJECT) {
			outputAttributes = getAllChildren(attribute.m_id);
		} else {
			outputAttributes.push_back(attribute);
		}

		return outputAttributes;
	}

	/**
	 * used in conjunction with set. Finds the attribute whose value needs to be set, or creates it if it doesn't exist
	*/
	easyJson& key (string key) {
		//duplicated code from set() - put in a func
		int searchParent = m_baseId;
		if (m_setId >= 0) {
			searchParent = m_setId;
		}
		for (int i = 0; i < m_jsonAttributes.size(); i++) {
			Attribute currentAttribute = m_jsonAttributes[i];
			if (currentAttribute.m_key == key && currentAttribute.m_parentId == searchParent) {
				m_setId = currentAttribute.m_id;	//not thread safe but none of it is probably
				return *this;
			}
		}
		//didn't find existing attribute with given key - create a new attribute
		Attribute newAttribute;
		newAttribute.m_id = m_currentId;
		m_currentId++;
		newAttribute.m_parentId = searchParent;
		newAttribute.m_key = key;
		newAttribute.saveValue("", newAttribute.OBJECT);
		m_setId = newAttribute.m_id;
		int parentPos = 0;
		for (int i = 0; i < m_jsonAttributes.size(); i++) {
			if (m_jsonAttributes[i].m_id == searchParent) {
				int parentPos = i;
			}
		}
		m_jsonAttributes.insert(m_jsonAttributes.begin() + parentPos, newAttribute);
		return *this;
	}

	/**
	 * used in conjunction with set. Finds the attribute whose value needs to be set, or creates it if it doesn't exist
	*/
	easyJson& key (int index) {
		//duplicated code from set() - put in a func
		int searchParent = m_baseId;
		if (m_setId >= 0) {
			searchParent = m_setId;
		}
		int firstElement = getAttributeIndex(searchParent) + 1;
		//check if parent is an array?? 
		for (int i = firstElement; i <= firstElement + index; i++) {	//should it be parentIndex+1 ?
			if (m_jsonAttributes[i].m_parentId != searchParent) {
				Attribute newAttribute;
				newAttribute.m_id = m_currentId;
				m_currentId++;
				newAttribute.m_parentId = searchParent;
				newAttribute.saveValue("null", newAttribute.EMPTY);
				m_jsonAttributes.insert(m_jsonAttributes.begin() + firstElement + i , newAttribute);
			}
			m_setId = m_jsonAttributes[i].m_id;
		}

		return *this;
	}

	bool set (string value) {
		//again this should be a func
		for (int i = 0; i < m_jsonAttributes.size(); i++) {
			Attribute currentAttribute = m_jsonAttributes[i];
			if (m_jsonAttributes[i].m_id == m_setId) {
				m_jsonAttributes[i].saveValue(value);
				deleteAllChildren(m_setId);
				m_setId = -1;
				return true;
			}
		}

		return false;
	}

	int getType() {
		return m_jsonAttributes[0].m_valueType;
	}

	bool getBool() {
		if(!m_jsonAttributes[0].m_valueType == Attribute::valueType::BOOL) throw invalid_argument("tried to call getBool on a json attribute that is not a bool");
		return m_jsonAttributes[0].m_value == "true";
	}

	string getString() {
		if(!m_jsonAttributes[0].m_valueType == Attribute::valueType::STRING) throw invalid_argument("tried to call getString on a json attribute that is not a string");
		return m_jsonAttributes[0].m_value;
	}

	float getFloat() {
		if(!m_jsonAttributes[0].m_valueType == Attribute::valueType::STRING) throw invalid_argument("tried to call getFloat on a json attribute that is not a number");
		return stof(m_jsonAttributes[0].m_value);
	}
};


int main() {

string basic = "{\"name\": \"charlie\", \"skills\": true, \"age\": 27}";
string midJson = "{\"name\": \"charlie\", \"skills\": {\"drawing\": true, \"writing\": \"false\"}, \"driver\": \"yes\"}";
string arrJson = "{\"name\": \"charlie\", \"skills\": [true, true, false], \"drives\": \"yes\"}";
string bigJson = "{ \"name\":\"charlie\", \"age\":24, \"parents\": { \"mother\": true, \"father\": { \"name\": \"steve\", \"age\": \"50\" }}, \"dob\": \"123456\"}";
string numberJson = "{\"name\": \"charlie\", \"int\": 5, \"float\": 0.4214231, \"exp\": 8e6}";
string arrayBase = "[true, true, false]";

//read json from file
ifstream stream("./json-examples/test.json");
easyJson fileJson(stream);
stream.close();

//save json to file
ofstream ostream("./json-examples/test-out.json");
string output = fileJson.generateJsonString(); //maybe its weird to go via string but currently using strings in my algos so
ostream << output;
ostream.close();

//read json from string literal
easyJson myJson(basic);

//compare input to output
cout << myJson.m_jsonString << endl;
cout << myJson.generateJsonString() << endl;

//set json values - object
myJson.key("skills").set("false");
myJson.key("age").set("27");

//get json object
easyJson skills = myJson.get("skills");

//set json values - array
fileJson.key("answers").key(2).set("true");

//get json array value
easyJson answers = fileJson.get("answers").get(2);

//get json value
bool answer;
if (answers.getType() == Attribute::valueType::BOOL) {
	answer = answers.getBool();
}

}