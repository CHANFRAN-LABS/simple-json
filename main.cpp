#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <algorithm>
#include <iterator>
#include <set>
#include <list>
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

	/**
	 * @fn Attribute() 
	 * constructor for Attribute class
	 * 
	*/
	Attribute() {
		string m_key = "";
		string m_value = "";
		int m_valueType = EMPTY;
		Attribute* m_nextElement = nullptr;
		Attribute* m_parentElement = nullptr;
		Attribute* m_childElement = nullptr;
		Attribute* m_prevElement = nullptr;
	}

	string m_key;
	string m_value;
	int m_valueType = EMPTY;
	Attribute* m_nextElement = nullptr;
	Attribute* m_parentElement = nullptr;
	Attribute* m_childElement = nullptr;
	Attribute* m_prevElement = nullptr;

	Attribute* getNext() {
		return m_nextElement;
	}

	Attribute* getParent() {
		return m_parentElement;
	}

	Attribute* getChild() {
		return m_childElement;
	}

	void copyChild() {
		Attribute* newAttribute = new Attribute;
		*newAttribute = *m_childElement;
		m_childElement = newAttribute;
		m_childElement->m_parentElement = this;
	}

	void copyNext() {
		Attribute* newAttribute = new Attribute;
		*newAttribute = *m_nextElement;
		m_nextElement = newAttribute;
		m_nextElement->m_parentElement = m_parentElement;
	}

	/**
	 * when get() is called on a parent element we need to create a copy of the relevant branch of the original json object
	 * this function cleans the values of the old parent element so that it acts as the new base element
	 */
	void cleanFirstElement() {
		m_key = "";
		m_value = "";
		m_nextElement = nullptr;	//set next element to empty attribute to mark the end of the linked list
		m_parentElement = nullptr;
	}

	/**
	 * when get() is called on a primitive element (i.e. not object or array) we need to remove the key and all connected elements
	 */
	void cleanOnlyElement() {
		m_key = "";
		m_nextElement = nullptr;
		m_parentElement = nullptr;
		m_childElement = nullptr;
		m_prevElement = nullptr;
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
		} else if (value == "true" || value == "false") {
			m_valueType = BOOL;
		} else if (value == "null") {
			m_valueType = EMPTY;
		} else if (isFloat(value)) {
			m_valueType = NUMBER;
		} else {
			throw invalid_argument("invalid json value");
			//value is not a valid json value
		}
	}

	/**
	 * what is this for?
	 */
	string getKey() {
		switch (m_parentElement->m_valueType) {
			case ARRAY:
				return "";
			case OBJECT:
				return "\"" + m_key + "\": ";
			default:
				return "";
				//throw error?
		}
	}

	string getValue() {
		if (m_valueType == STRING) return "\"" + m_value + "\"";
		return m_value;
	}

	string getOpenBracket() {
		switch (m_valueType) {
			case OBJECT:
				return "{";
			case ARRAY:
				return "[";
			default:
				//throw error?
				return "";
		}
	}

	string getCloseBracket() {
		switch (m_valueType) {
			case OBJECT:
				return "}";
			case ARRAY:
				return "]";
			default:
				//throw error?
				return "";
		}
	}

	void saveKey(string key) {
		m_key = key;
	}

	static bool isFloat( string value ) {
		istringstream testStream(value);
		float testFloat;
		testStream >> noskipws >> testFloat;
		return testStream.eof() && !testStream.fail(); 
	}

	/**
	 * set the value of the element identified by key() to a bool 
	*/
	void setBool(bool value) {
		string strValue = value ? "true" : "false";
		saveValue(strValue);
	}

	/**
	 * set the value of the element identified by key() to a string 
	*/
	void setString(string value) {
		saveValue("\"" + value + "\"");
	}

	/**
	 * set the value of the element identified by key() to a float 
	*/
	void setFloat(float value) {
		saveValue(to_string(value));
	}

	/**
	 * set the value of the element identified by key() to null
	*/
	void setNull() {
		saveValue("null");
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

	string m_jsonString;
	string m_cleanString;
	string m_parseString;
	bool m_exitingParent = false;
	Attribute* m_firstElement;
	list<Attribute*> m_pElements;

	/**
	 * constructor - used when parsing a json string
	*/
	easyJson(string input) {
		cleanAndParse(input);
	}

	/**
	 * constructor - used when copying an existing easyJson object to a new one
	 */
	easyJson (Attribute* baseElement) {
		if (!baseElement) throw invalid_argument("tried to create a json object with NULL first element");
		m_firstElement = new Attribute();
		*m_firstElement = *(baseElement);
		if (isPrimitiveJson()) {
			m_firstElement->cleanOnlyElement();
			m_pElements.push_back(m_firstElement);
			m_jsonString = generateJsonString();
		} else {
			m_firstElement->cleanFirstElement();
			walkAndCopy();
			m_jsonString = generateJsonString();
		}
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
	 * destructor - deletes all elements of the json object
	 */
	~easyJson() {
		for (Attribute* element:m_pElements)
			delete element;
		for (Proxy* proxy:m_pProxys)
			delete proxy;
		m_pElements.clear();
	}

	//----------------------------- DATA STRUCTURE METHODS ------------------------------//

	/**
	 * @brief used to identify if the json is just a singular primitive value, i.e. is just a string/bool/number
	 */
	bool isPrimitiveJson() {
		return m_firstElement->m_valueType != Attribute::valueType::OBJECT && m_firstElement->m_valueType != Attribute::valueType::ARRAY;
	}

	/**
	 * @param attribute {Attribute} - the attribute to be saved into the attribute vector
	 * this func saves the current attribute into the attribute vector then returns a blank attribute
	 * @return a blank new Attribute to be used for the next step in the json parse
	 * 
	*/
	Attribute* addAttribute (Attribute* pCurrentAttribute) {
		Attribute* pNewAttribute = new Attribute();
		pCurrentAttribute->m_nextElement = pNewAttribute;
		pNewAttribute->m_parentElement = pCurrentAttribute->m_parentElement;
		m_pElements.push_back(pNewAttribute);
		return pNewAttribute;
	}

	Attribute* addParent (Attribute* pCurrentAttribute) {
		Attribute* pNewAttribute = new Attribute();
		pCurrentAttribute->m_childElement = pNewAttribute;
		pNewAttribute->m_parentElement = pCurrentAttribute;
		m_pElements.push_back(pNewAttribute);
		return pNewAttribute;
	}

	Attribute* addLastChild (Attribute* pCurrentAttribute) {
		Attribute* pNewAttribute = new Attribute();
		pNewAttribute->m_parentElement = pCurrentAttribute->m_parentElement->m_parentElement;
		pNewAttribute->m_prevElement = pCurrentAttribute->m_parentElement;
		pCurrentAttribute->m_parentElement->m_nextElement = pNewAttribute;
		m_pElements.push_back(pNewAttribute);
		return pNewAttribute;
		//instead of the conditionals in , } ] cases for parse, we could use addLastChild a bit like findNextElement for the generate, where it looks for }] and basically does move next up each time, until it hits comma
	}

	/**
	 * @brief checks if we have returned to the first element, at which point we have finished parsing the string
	 */
	bool backToStart(Attribute* pCurrentAttribute) {
		return pCurrentAttribute->getParent() == m_firstElement;
	}

	/**
	 * @brief If exiting more than one parent in a row in generateJsonString, we need to move the element we created up to the next parent
	 */
	void moveNextUp (Attribute* pCurrentAttribute) {
		pCurrentAttribute->m_prevElement->m_nextElement = nullptr;
		if (backToStart(pCurrentAttribute)) {
			//we have already reached the end of the list - delete the new attribute
			m_pElements.remove(pCurrentAttribute);
			delete pCurrentAttribute;
			return;
		}
		pCurrentAttribute->m_parentElement->m_nextElement = pCurrentAttribute;
		pCurrentAttribute->m_parentElement = pCurrentAttribute->m_parentElement->m_parentElement;
	}

	/**
	 * Travereses backwards up the linked list when a layer end is reached to find the next element to append to the json string
	 * Adds a closing bracket each time because each layer represents another parent closed
	 */
	Attribute* walkBackwards(Attribute* pAttribute) {
		while (pAttribute) {
			pAttribute = pAttribute->getParent();
			if (!pAttribute) return nullptr;	//end of list reached
			if (pAttribute->getNext()) {
				return pAttribute->getNext();
			}
		}
		return nullptr;
	}

	void walkAndCopy() {
		Attribute* pAttribute = m_firstElement;
		while(pAttribute) {
			if (pAttribute->getChild()) {
				pAttribute->copyChild();
				m_pElements.push_back(pAttribute);
				pAttribute = pAttribute->getChild();
			} else if (pAttribute->getNext()) {
				pAttribute->copyNext();
				m_pElements.push_back(pAttribute);
				pAttribute = pAttribute->getNext();
			} else if (pAttribute->getParent()) {
				m_pElements.push_back(pAttribute);
				pAttribute = walkBackwards(pAttribute);
			} else {
				break;
			}
		}
	}

	//----------------------------- DESERIALISATION METHODS ------------------------------//

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

	void handleColon(Attribute* pAttribute) {
		int pos = m_parseString.find_first_of(':');
		pAttribute->saveKey(m_parseString.substr(1, pos-2));
		m_parseString.erase(0, pos+1);
	}

	Attribute* handleComma(Attribute* pAttribute) {
		int pos = m_parseString.find_first_of(',');
		if (m_exitingParent) {
			m_parseString.erase(0, pos+1);
			m_exitingParent = false;
			return pAttribute;
		} else {
			pAttribute->saveValue(m_parseString.substr(0, pos), pAttribute->EMPTY);
			m_parseString.erase(0, pos+1);
			return addAttribute(pAttribute);
		}
	}

	Attribute* handleOpenBracket(Attribute* pAttribute, Attribute::valueType valueType) {
		char delimiter = valueType == Attribute::OBJECT ? '{' : '[';
		int pos = m_parseString.find_first_of(delimiter);
		pAttribute->saveValue("", valueType);
		m_parseString.erase(0, pos+1);
		return addParent(pAttribute);
	}

	Attribute* handleCloseBracket(Attribute* pAttribute, Attribute::valueType valueType) {
		char delimiter = valueType == Attribute::OBJECT ? '}' : ']';
		int pos = m_parseString.find_first_of(delimiter);
		if (m_exitingParent) {
			moveNextUp(pAttribute);
			m_parseString.erase(0, pos+1);
			return pAttribute;
		} else {
			pAttribute->saveValue(m_parseString.substr(0, pos), Attribute::valueType::EMPTY);
			m_parseString.erase(0, pos+1);
			if (backToStart(pAttribute)) return pAttribute;
			m_exitingParent = true;
			return addLastChild(pAttribute);
		}
	}

	/**
	 * parse json string, searching for special characters and generating attributes to represent the string as a json object
	 * attribute IDs are used to link parents to children
	*/
	void parseJsonString(string input) {
		m_parseString = input;
		m_exitingParent = false;
		char delimiter;

		Attribute* pAttribute = new Attribute;
		m_firstElement = pAttribute;
		m_pElements.push_back(pAttribute);

		while (m_parseString != "") {
			delimiter = findNextDelimiter(m_parseString);
			switch (delimiter) {
				case ':':
					handleColon(pAttribute);
					break;

				case ',':
					pAttribute = handleComma(pAttribute);
					break;

				case '{':
					pAttribute = handleOpenBracket(pAttribute, Attribute::valueType::OBJECT);
					break;
				
				case '}':
				{
					pAttribute = handleCloseBracket(pAttribute, Attribute::valueType::OBJECT);
					break;
				}

				case '[':
				{
					pAttribute = handleOpenBracket(pAttribute, Attribute::valueType::ARRAY);
					break;
				}

				case ']':
				{
					pAttribute = handleCloseBracket(pAttribute, Attribute::valueType::ARRAY);
					break;
				}

				default:
						throw invalid_argument("string is not a valid json");
			}
		}
	}

	//----------------------------- SERIALISATION METHODS ------------------------------//

	//need to look at this - surely shouldn't be neccessary
	string removeLeadingTrailing(string input) {
		return input.substr(4, input.length()-4);
	}

	/**
	 * Travereses backwards up the linked list when a layer end is reached to find the next element to append to the json string
	 * Adds a closing bracket each time because each layer represents another parent closed
	 */
	Attribute* findNextElement(Attribute* pAttribute, string &output) {
		while (pAttribute) {
			pAttribute = pAttribute->getParent();
			if (!pAttribute) return nullptr;	//reached end of list
			output.append(pAttribute->getCloseBracket());
			if (pAttribute->getNext()) {
				output.append(", ");
				return pAttribute->getNext();
			}
		}
		return nullptr;
	}

	/**
	 * @brief generate string representation of the json attributes - ready to be saved to file
	 */
	string generateJsonString () {
		string output;
		Attribute* pAttribute = m_firstElement;
		if (isPrimitiveJson()) {
			return m_firstElement->getValue();
		}
		bool exitingParent = false;
		while(pAttribute) {
			Attribute currentAttribute = *pAttribute;
			if (currentAttribute.getChild()) {
				output.append("\"" + currentAttribute.m_key + "\": " + currentAttribute.getOpenBracket()); //could put the first bit in getKey by returning the non empty option if (getChild)
				pAttribute = currentAttribute.getChild();
			} else if (currentAttribute.getNext()) {
				output.append(currentAttribute.getKey() + currentAttribute.getValue() + ", ");
				pAttribute = currentAttribute.getNext();
			} else if (currentAttribute.getParent()) {
				output.append(currentAttribute.getKey() + currentAttribute.getValue() + " ");
				pAttribute = findNextElement(pAttribute, output);
			} else {
				break;
			}
		}
		output = removeLeadingTrailing(output);
		return output;
	}

	//----------------------------- GET METHODS ------------------------------//

	Attribute* getElement(string key) {
		Attribute* pAttribute = m_firstElement->m_childElement;
		while(pAttribute) {
			if (pAttribute->m_key == key) return pAttribute;
			pAttribute = pAttribute->getNext();
		}
		return nullptr;
	}

	Attribute* getElement(int index) {
		Attribute* pAttribute = m_firstElement->m_childElement;
		int current = 0;
		while(pAttribute) {
			if (current == index) return pAttribute;
			pAttribute = pAttribute->getNext();
			current++;
		}
		return nullptr;
	}

	/**
	 * query json object by key
	 * return Attribute so that the object is constructed and assigned outside the object
	*/
	easyJson get (string key) {
		if (m_firstElement->m_valueType == Attribute::valueType::ARRAY) throw invalid_argument("cannot get an array by key");
		Attribute* firstElement = getElement(key);
		if (!firstElement) return NULL;
		return firstElement;
	}

	/**
	 * query json array by index
	 * return Attribute so that the object is constructed and assigned outside the object
	*/
	easyJson get (int index) {
		if (m_firstElement->m_valueType == Attribute::valueType::OBJECT) throw invalid_argument("cannot get an object by index");
		Attribute* firstElement = getElement(index);
		if (!firstElement) return NULL;
		return firstElement;
	}

	bool isBool() {
		return m_firstElement->m_valueType == Attribute::valueType::BOOL;
	}

	bool getBool() {
		if(m_firstElement->m_valueType != Attribute::valueType::BOOL) throw invalid_argument("element is not a bool");
		return m_firstElement->m_value == "true";
	}

	bool isString() {
		return m_firstElement->m_valueType == Attribute::valueType::STRING;
	}

	string getString() {
		if(m_firstElement->m_valueType != Attribute::valueType::STRING) throw invalid_argument("element is not a string");
		return m_firstElement->getValue();
	}

	bool isFloat() {
		return m_firstElement->m_valueType == Attribute::valueType::NUMBER;
	}

	float getFloat() {
		if(m_firstElement->m_valueType != Attribute::valueType::NUMBER) throw invalid_argument("element is not a number");
		return stof(m_firstElement->getValue());
	}

	//----------------------------- SET METHODS ------------------------------//

	/**
	 * @brief lookup element to be set by key - if not found add a new element
	 */
	Attribute* findOrAddElement(Attribute* startingElement, string key) {
		Attribute* pAttribute = m_firstElement->getChild();
		if (startingElement) pAttribute = startingElement->getChild();
		while(pAttribute) {
			if (pAttribute->m_key == key) return pAttribute;
			if (pAttribute->getNext()) {
				pAttribute = pAttribute->getNext();
			} else {
				pAttribute = addAttribute(pAttribute);
				pAttribute->saveKey(key);
				return pAttribute;
			}
		}
		return nullptr;
	}

	/**
	 * @brief lookup element to be set by index - if not found add a new element at the specified index
	 */
	Attribute* findOrAddElement(Attribute* startingElement, int index) {
		Attribute* pAttribute = m_firstElement->getChild();
		if (startingElement) pAttribute = startingElement->getChild();
		int current = 0;
		while(pAttribute) {
			if (current == index) return pAttribute;
			if (pAttribute->getNext()) {
				pAttribute = pAttribute->getNext();
			} else {
				if (current == index) return addAttribute(pAttribute);
				//add empty elements until we reach the specified index
				pAttribute = addAttribute(pAttribute);
				pAttribute->saveValue("null");
			}
			current++;
		}
		return nullptr;
	}

	/**
	 * proxy class - intermediate class used to store pointer to queired attribute during key() > set()
	 */
	class Proxy
	{
	public:
		easyJson& m_json;
		Attribute* m_element = nullptr;
		Proxy(easyJson& json) : m_json(json) {}
		Proxy& key(string key) {
			m_element = m_json.findOrAddElement(m_element, key);
			if (!m_element) throw invalid_argument("could not find or create this key");
			return *this;
		}
		Proxy& key(int index) {
			m_element = m_json.findOrAddElement(m_element, index);
			if (!m_element) throw invalid_argument("could not find or create this index");
			return *this;
		}
		void setBool(bool value) {
			m_element->setBool(value);
		}		
		void setString(string value) {
			m_element->setString(value);
		}
		void setFloat(float value) {
			m_element->setFloat(value);
		}
	};

	list<Proxy*> m_pProxys;

	/**
	 * find by key the element which should be set in subsequent call to set()
	 * returns a temporary proxy object which stores a pointer to the value to be set
	*/
	Proxy key (string key) {
		Proxy* pProxy = new Proxy(*this);
		m_pProxys.push_back(pProxy);
		return pProxy->key(key);
	}

	/**
	 * find by index the element which should be set in subsequent call to set()
	 * returns a temporary proxy object which stores a pointer to the value to be set
	*/
	Proxy key(int index) {
		Proxy* pProxy = new Proxy(*this);
		m_pProxys.push_back(pProxy);
		return pProxy->key(index);
	}
};



int main() {
string example = "{\"person\": {\"name\": \"charlie\", \"skills\": true, \"age\": 27}}";
string arrayExample = "{\"name\": \"charlie\", \"skills\": [5, \"drawing\", false], \"drives\": \"yes\"}";
string largeExample = "{ \"name\":\"charlie\", \"age\":24, \"parents\": { \"mother\": true, \"father\": { \"name\": \"steve\", \"age\": \"50\" }}, \"dob\": \"123456\"}";

//read json from file
ifstream stream("./json-examples/test.json");
easyJson fileJson(stream);
stream.close();

//save json to file
ofstream ostream("./json-examples/test-out.json");
ostream << fileJson.generateJsonString();
ostream.close();

//read json from string literal
easyJson exampleJson(example);

//compare input string literal to serialised json object
cout << exampleJson.m_jsonString << endl;
cout << exampleJson.generateJsonString() << endl;

//get json object
easyJson person = exampleJson.get("person");

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
easyJson individual(arrayExample);

//get json array
easyJson my_skills = individual.get("skills");

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