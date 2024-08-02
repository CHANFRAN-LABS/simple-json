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
 * @class Element
 * store invididual Elements of the json object, plus information about their parent Elements, and metadata
*/
class Element {
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
	 * @fn Element() 
	 * constructor for Element class
	 * 
	*/
	Element() {
		string m_key = "";
		string m_value = "";
		int m_valueType = EMPTY;
		Element* m_nextElement = nullptr;
		Element* m_parentElement = nullptr;
		Element* m_childElement = nullptr;
		Element* m_prevElement = nullptr;
	}

	string m_key;
	string m_value;
	int m_valueType = EMPTY;
	Element* m_nextElement = nullptr;
	Element* m_parentElement = nullptr;
	Element* m_childElement = nullptr;
	Element* m_prevElement = nullptr;

	Element* getNext() {
		return m_nextElement;
	}

	Element* getParent() {
		return m_parentElement;
	}

	Element* getChild() {
		return m_childElement;
	}

	void copyChild() {
		Element* newElement = new Element;
		*newElement = *m_childElement;
		m_childElement = newElement;
		m_childElement->m_parentElement = this;
	}

	void copyNext() {
		Element* newElement = new Element;
		*newElement = *m_nextElement;
		m_nextElement = newElement;
		m_nextElement->m_parentElement = m_parentElement;
	}

	/**
	 * when get() is called on a parent element we need to create a copy of the relevant branch of the original json object
	 * this function cleans the values of the old parent element so that it acts as the new base element
	 */
	void cleanFirstElement() {
		m_key = "";
		m_value = "";
		m_nextElement = nullptr;	//set next element to empty Element to mark the end of the linked list
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
 * Stores json object as a vector of Elements
 * Parses json text and stores resulting json object as a vector of Elements
 * Allows value access via [] operator
 * plus more tbc
*/
class easyJson {
public:

	string m_jsonString;
	string m_cleanString;
	string m_parseString;
	bool m_exitingParent = false;
	Element* m_firstElement;
	list<Element*> m_pElements;

	/**
	 * constructor - used when parsing a json string
	*/
	easyJson(string input) {
		cleanAndParse(input);
	}

	/**
	 * constructor - used when copying an existing easyJson object to a new one
	 */
	easyJson (Element* baseElement) {
		if (!baseElement) throw invalid_argument("tried to create a json object with NULL first element");
		m_firstElement = new Element();
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
		for (Element* element:m_pElements)
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
		return m_firstElement->m_valueType != Element::valueType::OBJECT && m_firstElement->m_valueType != Element::valueType::ARRAY;
	}

	/**
	 * @param Element {Element} - the Element to be saved into the Element vector
	 * this func saves the current Element into the Element vector then returns a blank Element
	 * @return a blank new Element to be used for the next step in the json parse
	 * 
	*/
	Element* addElement (Element* pCurrentElement) {
		Element* pNewElement = new Element();
		pCurrentElement->m_nextElement = pNewElement;
		pNewElement->m_parentElement = pCurrentElement->m_parentElement;
		m_pElements.push_back(pNewElement);
		return pNewElement;
	}

	Element* addParent (Element* pCurrentElement) {
		Element* pNewElement = new Element();
		pCurrentElement->m_childElement = pNewElement;
		pNewElement->m_parentElement = pCurrentElement;
		m_pElements.push_back(pNewElement);
		return pNewElement;
	}

	Element* addLastChild (Element* pCurrentElement) {
		Element* pNewElement = new Element();
		pNewElement->m_parentElement = pCurrentElement->m_parentElement->m_parentElement;
		pNewElement->m_prevElement = pCurrentElement->m_parentElement;
		pCurrentElement->m_parentElement->m_nextElement = pNewElement;
		m_pElements.push_back(pNewElement);
		return pNewElement;
		//instead of the conditionals in , } ] cases for parse, we could use addLastChild a bit like findNextElement for the generate, where it looks for }] and basically does move next up each time, until it hits comma
	}

	/**
	 * @brief checks if we have returned to the first element, at which point we have finished parsing the string
	 */
	bool backToStart(Element* pCurrentElement) {
		return pCurrentElement->getParent() == m_firstElement;
	}

	/**
	 * @brief If exiting more than one parent in a row in generateJsonString, we need to move the element we created up to the next parent
	 */
	void moveNextUp (Element* pCurrentElement) {
		pCurrentElement->m_prevElement->m_nextElement = nullptr;
		if (backToStart(pCurrentElement)) {
			//we have already reached the end of the list - delete the new element
			m_pElements.remove(pCurrentElement);
			delete pCurrentElement;
			return;
		}
		pCurrentElement->m_parentElement->m_nextElement = pCurrentElement;
		pCurrentElement->m_parentElement = pCurrentElement->m_parentElement->m_parentElement;
	}

	/**
	 * Travereses backwards up the linked list when a layer end is reached to find the next element to append to the json string
	 * Adds a closing bracket each time because each layer represents another parent closed
	 */
	Element* walkBackwards(Element* pElement) {
		while (pElement) {
			pElement = pElement->getParent();
			if (!pElement) return nullptr;	//end of list reached
			if (pElement->getNext()) {
				return pElement->getNext();
			}
		}
		return nullptr;
	}

	void walkAndCopy() {
		Element* pElement = m_firstElement;
		while(pElement) {
			if (pElement->getChild()) {
				pElement->copyChild();
				m_pElements.push_back(pElement);
				pElement = pElement->getChild();
			} else if (pElement->getNext()) {
				pElement->copyNext();
				m_pElements.push_back(pElement);
				pElement = pElement->getNext();
			} else if (pElement->getParent()) {
				m_pElements.push_back(pElement);
				pElement = walkBackwards(pElement);
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
	 * finds next special character in json string to denote start / end of current element
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

		throw invalid_argument("string is not a valid json");
	}

	void handleColon(Element* pElement) {
		int pos = m_parseString.find_first_of(':');
		pElement->saveKey(m_parseString.substr(1, pos-2));
		m_parseString.erase(0, pos+1);
	}

	Element* handleComma(Element* pElement) {
		int pos = m_parseString.find_first_of(',');
		if (m_exitingParent) {
			m_parseString.erase(0, pos+1);
			m_exitingParent = false;
			return pElement;
		} else {
			pElement->saveValue(m_parseString.substr(0, pos), pElement->EMPTY);
			m_parseString.erase(0, pos+1);
			return addElement(pElement);
		}
	}

	Element* handleOpenBracket(Element* pElement, Element::valueType valueType) {
		char delimiter = valueType == Element::OBJECT ? '{' : '[';
		int pos = m_parseString.find_first_of(delimiter);
		pElement->saveValue("", valueType);
		m_parseString.erase(0, pos+1);
		return addParent(pElement);
	}

	Element* handleCloseBracket(Element* pElement, Element::valueType valueType) {
		char delimiter = valueType == Element::OBJECT ? '}' : ']';
		int pos = m_parseString.find_first_of(delimiter);
		if (m_exitingParent) {
			moveNextUp(pElement);
			m_parseString.erase(0, pos+1);
			return pElement;
		} else {
			pElement->saveValue(m_parseString.substr(0, pos), Element::valueType::EMPTY);
			m_parseString.erase(0, pos+1);
			if (backToStart(pElement)) return pElement;
			m_exitingParent = true;
			return addLastChild(pElement);
		}
	}

	/**
	 * parse json string, searching for special characters and generating elements to represent the string as a json object
	 * element IDs are used to link parents to children
	*/
	void parseJsonString(string input) {
		m_parseString = input;
		m_exitingParent = false;
		char delimiter;

		Element* pElement = new Element;
		m_firstElement = pElement;
		m_pElements.push_back(pElement);

		while (m_parseString != "") {
			delimiter = findNextDelimiter(m_parseString);
			switch (delimiter) {
				case ':':
					handleColon(pElement);
					break;

				case ',':
					pElement = handleComma(pElement);
					break;

				case '{':
					pElement = handleOpenBracket(pElement, Element::valueType::OBJECT);
					break;
				
				case '}':
				{
					pElement = handleCloseBracket(pElement, Element::valueType::OBJECT);
					break;
				}

				case '[':
				{
					pElement = handleOpenBracket(pElement, Element::valueType::ARRAY);
					break;
				}

				case ']':
				{
					pElement = handleCloseBracket(pElement, Element::valueType::ARRAY);
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
	Element* findNextElement(Element* pElement, string &output) {
		while (pElement) {
			pElement = pElement->getParent();
			if (!pElement) return nullptr;	//reached end of list
			output.append(pElement->getCloseBracket());
			if (pElement->getNext()) {
				output.append(", ");
				return pElement->getNext();
			}
		}
		return nullptr;
	}

	/**
	 * @brief generate string representation of the json elements - ready to be saved to file
	 */
	string generateJsonString () {
		string output;
		Element* pElement = m_firstElement;
		if (isPrimitiveJson()) {
			return m_firstElement->getValue();
		}
		bool exitingParent = false;
		while(pElement) {
			Element currentElement = *pElement;
			if (currentElement.getChild()) {
				output.append("\"" + currentElement.m_key + "\": " + currentElement.getOpenBracket()); //could put the first bit in getKey by returning the non empty option if (getChild)
				pElement = currentElement.getChild();
			} else if (currentElement.getNext()) {
				output.append(currentElement.getKey() + currentElement.getValue() + ", ");
				pElement = currentElement.getNext();
			} else if (currentElement.getParent()) {
				output.append(currentElement.getKey() + currentElement.getValue() + " ");
				pElement = findNextElement(pElement, output);
			} else {
				break;
			}
		}
		output = removeLeadingTrailing(output);
		return output;
	}

	//----------------------------- GET METHODS ------------------------------//

	Element* getElement(string key) {
		Element* pElement = m_firstElement->m_childElement;
		while(pElement) {
			if (pElement->m_key == key) return pElement;
			pElement = pElement->getNext();
		}
		return nullptr;
	}

	Element* getElement(int index) {
		Element* pElement = m_firstElement->m_childElement;
		int current = 0;
		while(pElement) {
			if (current == index) return pElement;
			pElement = pElement->getNext();
			current++;
		}
		return nullptr;
	}

	/**
	 * query json object by key
	 * return Element so that the object is constructed and assigned outside the object
	*/
	easyJson get (string key) {
		if (m_firstElement->m_valueType == Element::valueType::ARRAY) throw invalid_argument("cannot get an array by key");
		Element* firstElement = getElement(key);
		if (!firstElement) return NULL;
		return firstElement;
	}

	/**
	 * query json array by index
	 * return Element so that the object is constructed and assigned outside the object
	*/
	easyJson get (int index) {
		if (m_firstElement->m_valueType == Element::valueType::OBJECT) throw invalid_argument("cannot get an object by index");
		Element* firstElement = getElement(index);
		if (!firstElement) return NULL;
		return firstElement;
	}

	bool isBool() {
		return m_firstElement->m_valueType == Element::valueType::BOOL;
	}

	bool getBool() {
		if(m_firstElement->m_valueType != Element::valueType::BOOL) throw invalid_argument("element is not a bool");
		return m_firstElement->m_value == "true";
	}

	bool isString() {
		return m_firstElement->m_valueType == Element::valueType::STRING;
	}

	string getString() {
		if(m_firstElement->m_valueType != Element::valueType::STRING) throw invalid_argument("element is not a string");
		return m_firstElement->getValue();
	}

	bool isFloat() {
		return m_firstElement->m_valueType == Element::valueType::NUMBER;
	}

	float getFloat() {
		if(m_firstElement->m_valueType != Element::valueType::NUMBER) throw invalid_argument("element is not a number");
		return stof(m_firstElement->getValue());
	}

	//----------------------------- SET METHODS ------------------------------//

	/**
	 * @brief lookup element to be set by key - if not found add a new element
	 */
	Element* findOrAddElement(Element* startingElement, string key) {
		Element* pElement = m_firstElement->getChild();
		if (startingElement) pElement = startingElement->getChild();
		while(pElement) {
			if (pElement->m_key == key) return pElement;
			if (pElement->getNext()) {
				pElement = pElement->getNext();
			} else {
				pElement = addElement(pElement);
				pElement->saveKey(key);
				return pElement;
			}
		}
		return nullptr;
	}

	/**
	 * @brief lookup element to be set by index - if not found add a new element at the specified index
	 */
	Element* findOrAddElement(Element* startingElement, int index) {
		Element* pElement = m_firstElement->getChild();
		if (startingElement) pElement = startingElement->getChild();
		int current = 0;
		while(pElement) {
			if (current == index) return pElement;
			if (pElement->getNext()) {
				pElement = pElement->getNext();
			} else {
				if (current == index) return addElement(pElement);
				//add empty elements until we reach the specified index
				pElement = addElement(pElement);
				pElement->saveValue("null");
			}
			current++;
		}
		return nullptr;
	}

	/**
	 * proxy class - intermediate class used to store pointer to queired element during key() > set()
	 */
	class Proxy
	{
	public:
		easyJson& m_json;
		Element* m_element = nullptr;
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