#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <list>
#include <gtest/gtest.h>
using namespace std;

/**
 * @class Element
 * Data structure for storing individual elements of the json object
 * Multi-layer linked list node structure
*/
class Element {
friend class SimpleJson;
private:
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
	 * @brief constructor for Element class - initialise member variables
	*/
	Element() {
		string m_key = "";
		string m_value = "";
		int m_valueType = EMPTY;
		Element* m_pNextElement = nullptr;
		Element* m_pParentElement = nullptr;
		Element* m_pChildElement = nullptr;
		Element* m_pPrevElement = nullptr;
	}

	string m_key;
	string m_value;
	int m_valueType = EMPTY;
	Element* m_pNextElement = nullptr;
	Element* m_pParentElement = nullptr;
	Element* m_pChildElement = nullptr;
	Element* m_pPrevElement = nullptr;

	/**
	 * @brief return the next element
	 */
	Element* getNext() {
		return m_pNextElement;
	}

	/**
	 * @brief return the parent element
	 */
	Element* getParent() {
		return m_pParentElement;
	}

	/**
	 * @brief return the child element
	 */
	Element* getChild() {
		return m_pChildElement;
	}

	/**
	 * @brief copy child element to new pointer
	 */
	void copyChild() {
		Element* pNewElement = new Element;
		*pNewElement = *m_pChildElement;
		m_pChildElement = pNewElement;
		m_pChildElement->m_pParentElement = this;
	}

	/**
	 * @brief copy next element to new pointer
	 */
	void copyNext() {
		Element* pNewElement = new Element;
		*pNewElement = *m_pNextElement;
		m_pNextElement = pNewElement;
		m_pNextElement->m_pParentElement = m_pParentElement;
	}

	/**
	 * @brief when get() is called on a parent element we need to create a copy of the relevant branch of the original json object
	 * this function cleans the values of the old parent element so that it acts as the new base element
	 */
	void cleanFirstElement() {
		m_key = "";
		m_value = "";
		m_pNextElement = nullptr;	//set next element to null to mark the end of the new element tree
		m_pParentElement = nullptr;
	}

	/**
	 * @brief when get() is called on a primitive element (i.e. not object or array) we need to remove the key and all connected elements
	 */
	void cleanOnlyElement() {
		m_key = "";
		m_pNextElement = nullptr;
		m_pParentElement = nullptr;
		m_pChildElement = nullptr;
		m_pPrevElement = nullptr;
	}

	/**
	 * @brief set the value and valueType of the element. In some cases we need to specify the valueType, in others we infer it from the value itself
	*/
	void setValue(string value, int type=UNKNOWN) {
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
			throw invalid_argument("tried to set an invalid json value");
		}
	}

	/**
	 * @brief get the key from an element to append to json string during serialization
	 */
	string getKey() {
		switch (m_pParentElement->m_valueType) {
			case ARRAY:
				return "";
			case OBJECT:
				return "\"" + m_key + "\": ";
			default:
				throw invalid_argument("object structure corrupted");
		}
	}

	/**
	 * @brief get the value from an element - if valueType is string need to add ""
	 */
	string getValue() {
		if (m_valueType == STRING) return "\"" + m_value + "\"";
		return m_value;
	}

	/**
	 * @brief return a string containing the open bracket correpsonding to a parent element's value type
	 */
	string getOpenBracket() {
		switch (m_valueType) {
			case OBJECT:
				return "{";
			case ARRAY:
				return "[";
			default:
				throw invalid_argument("object structure corrupted");
		}
	}

	/**
	 * @brief return a string contianing the closing bracket correpsonding to a parent element's value type
	 */
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

	/**
	 * @brief set the key for an element
	 */
	void setKey(string key) {
		m_key = key;
	}

	/**
	 * @brief check if a given string can be treated as a number when saving elements value
	 */
	static bool isFloat( string value ) {
		istringstream testStream(value);
		float testFloat;
		testStream >> noskipws >> testFloat;
		return testStream.eof() && !testStream.fail(); 
	}

	/**
	 * @brief set the value of the element identified by key() to a bool 
	*/
	void setBool(bool value) {
		string strValue = value ? "true" : "false";
		setValue(strValue);
	}

	/**
	 * @brief set the value of the element identified by key() to a string 
	*/
	void setString(string value) {
		setValue("\"" + value + "\"");
	}

	/**
	 * @brief set the value of the element identified by key() to a float 
	*/
	void setFloat(float value) {
		setValue(to_string(value));
	}

	/**
	 * @brief set the value of the element identified by key() to null
	*/
	void setNull() {
		setValue("null");
	}
};



/**
 * @class SimpleJson
 * DOM style json object which stores json as a multi-layer linked list/tree of Elements
 * Contains serialization, deserialization, setting and getting methods
*/
class SimpleJson {
private:
	string m_jsonString;
	string m_cleanString;
	string m_parseString;
	bool m_exitingParent = false;
	bool m_backToStart = false;
	Element* m_pFirstElement;
	list<Element*> m_pElements;
public:
	/**
	 * @brief constructor - deserialize a json string
	*/
	SimpleJson(string input) {
		cleanAndParse(input);
	}

	/**
	 * @brief constructor - deserialize a json file
	 * @param stream - std::ifstream of file to be parsed
	*/
	SimpleJson(ifstream &stream) {
		string input((istreambuf_iterator<char>(stream)), istreambuf_iterator<char>());
		cleanAndParse(input);
	}

	/**
	 * @brief destructor - delete all elements of the json object
	 */
	~SimpleJson() {
		for (Element* element:m_pElements)
			delete element;
		for (Proxy* proxy:m_pProxys)
			delete proxy;
		m_pElements.clear();
	}
private:
	/**
	 * @brief constructor - create a new SimpleJson object from an existing one
	 */
	SimpleJson (Element* baseElement) {
		if (!baseElement) throw invalid_argument("tried to create a json object with NULL first element");
		m_pFirstElement = new Element();
		*m_pFirstElement = *(baseElement);
		if (isPrimitiveJson()) {
			m_pFirstElement->cleanOnlyElement();
			m_pElements.push_back(m_pFirstElement);
			m_jsonString = generateJsonString();
		} else {
			m_pFirstElement->cleanFirstElement();
			copyElementTree();
			m_jsonString = generateJsonString();
		}
	}

	//----------------------------- DATA STRUCTURE METHODS ------------------------------//

	/**
	 * @brief check if the json is just a singular primitive value, i.e. is just a string/bool/number
	 */
	bool isPrimitiveJson() {
		return m_pFirstElement->m_valueType != Element::valueType::OBJECT && m_pFirstElement->m_valueType != Element::valueType::ARRAY;
	}

	/**
	 * @brief save the current Element into the element tree and create a new element to be populated on the same branch
	*/
	Element* addElement (Element* pCurrentElement) {
		Element* pNewElement = new Element();
		pCurrentElement->m_pNextElement = pNewElement;
		pNewElement->m_pParentElement = pCurrentElement->m_pParentElement;
		m_pElements.push_back(pNewElement);
		return pNewElement;
	}

	/**
	 * @brief save the current Element to the element tree and create a new child element to be populated on a new branch
	 */
	Element* addChild (Element* pCurrentElement) {
		Element* pNewElement = new Element();
		pCurrentElement->m_pChildElement = pNewElement;
		pNewElement->m_pParentElement = pCurrentElement;
		m_pElements.push_back(pNewElement);
		return pNewElement;
	}

	/**
	 * @brief save the current Element to the element tree and close off a branch, then create a new element to be populated on the parent branch
	 */
	Element* addLastChild (Element* pCurrentElement) {
		Element* pNewElement = new Element();
		pNewElement->m_pParentElement = pCurrentElement->m_pParentElement->m_pParentElement;
		pNewElement->m_pPrevElement = pCurrentElement->m_pParentElement;
		pCurrentElement->m_pParentElement->m_pNextElement = pNewElement;
		m_pElements.push_back(pNewElement);
		return pNewElement;
	}

	/**
	 * @brief check if we have returned to the first element, at which point we have finished parsing the string
	 */
	bool backToStart(Element* pCurrentElement) {
		return pCurrentElement->getParent() == m_pFirstElement;
	}

	/**
	 * @brief If exiting more than one parent in a row in generateJsonString, we need to move the element we created up to the next parent
	 */
	void moveNextUp (Element* pCurrentElement) {
		pCurrentElement->m_pPrevElement->m_pNextElement = nullptr;
		if (backToStart(pCurrentElement)) {
			//we have reached the end of the list - delete the new element
			m_pElements.remove(pCurrentElement);
			delete pCurrentElement;
			m_backToStart = true;
			return;
		}
		pCurrentElement->m_pParentElement->m_pNextElement = pCurrentElement;
		pCurrentElement->m_pParentElement = pCurrentElement->m_pParentElement->m_pParentElement;
	}

	/**
	 * @brief when a branch ends, traverese backwards up the tree to find the next element to copy
	 */
	Element* exitBranch(Element* pElement) {
		while (pElement) {
			pElement = pElement->getParent();
			if (!pElement) return nullptr;	//end of list reached
			if (pElement->getNext()) {
				return pElement->getNext();
			}
		}
		return nullptr;
	}

	/**
	 * @brief create a copy of the element tree starting from a given first element
	 */
	void copyElementTree() {
		Element* pElement = m_pFirstElement;
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
				pElement = exitBranch(pElement);
			} else {
				break;
			}
		}
	}

	//----------------------------- DESERIALISATION METHODS ------------------------------//

	/**
	 * @brief remove spaces, linebreaks, tabs from string
	 */
	string removeWhitespace(string input) {
		input.erase(remove(input.begin(), input.end(), ' '), input.end());
		input.erase(remove(input.begin(), input.end(), '\n'), input.end());
		input.erase(remove(input.begin(), input.end(), '\t'), input.end());
		return input;
	}

	/**
	 * 
	 * @brief clean the input json string then deserialize it to build the element tree
	*/
	void cleanAndParse(string input) {
		m_jsonString = input;
		m_cleanString = removeWhitespace(input);
		parseJsonString(m_cleanString);
	}

	/**
	 * @brief find the next special character in json string
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

	/**
	 * @brief consume next colon to get the current element's key
	 */
	void handleColon(Element* pElement) {
		int pos = m_parseString.find_first_of(':');
		pElement->setKey(m_parseString.substr(1, pos-2));
		m_parseString.erase(0, pos+1);
	}

	/**
	 * @brief consume next comma to get the current elements value
	 * special case where the previous special char was a close bracket, in which case there is no value to populate
	 */
	Element* handleComma(Element* pElement) {
		int pos = m_parseString.find_first_of(',');
		if (m_exitingParent) {
			m_parseString.erase(0, pos+1);
			m_exitingParent = false;
			return pElement;
		} else {
			pElement->setValue(m_parseString.substr(0, pos), pElement->EMPTY);
			m_parseString.erase(0, pos+1);
			return addElement(pElement);
		}
	}

	/**
	 * @brief consume next open bracket to create a new branch of the element tree
	 */
	Element* handleOpenBracket(Element* pElement, Element::valueType valueType) {
		char delimiter = valueType == Element::OBJECT ? '{' : '[';
		int pos = m_parseString.find_first_of(delimiter);
		pElement->setValue("", valueType);
		m_parseString.erase(0, pos+1);
		return addChild(pElement);
	}

	/**
	 * @brief consume next close bracket to set the value of the current element and close off the current branch of the element tree
	 * special case if prev special char was a close bracket, in this case we move the new element up a layer
	 */
	Element* handleCloseBracket(Element* pElement, Element::valueType valueType) {
		char delimiter = valueType == Element::OBJECT ? '}' : ']';
		int pos = m_parseString.find_first_of(delimiter);
		if (m_exitingParent) {
			moveNextUp(pElement);
			m_parseString.erase(0, pos+1);
			return pElement;
		} else {
			pElement->setValue(m_parseString.substr(0, pos), Element::valueType::EMPTY);
			m_parseString.erase(0, pos+1);
			if (backToStart(pElement)) return pElement;
			m_exitingParent = true;
			return addLastChild(pElement);
		}
	}

	/**
	 * @brief deserialize a json string to create its representation as an element tree
	*/
	void parseJsonString(string input) {
		m_parseString = input;
		m_exitingParent = false;
		char delimiter;

		Element* pElement = new Element;
		m_pFirstElement = pElement;
		m_pElements.push_back(pElement);

		while (m_parseString != "" && !m_backToStart) {
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
					pElement = handleCloseBracket(pElement, Element::valueType::OBJECT);
					break;

				case '[':
					pElement = handleOpenBracket(pElement, Element::valueType::ARRAY);
					break;

				case ']':
					pElement = handleCloseBracket(pElement, Element::valueType::ARRAY);
					break;

				default:
						throw invalid_argument("string is not a valid json");
			}
		}
		if(m_parseString != "") throw invalid_argument("string is not a valid json");
	}

	//----------------------------- SERIALISATION METHODS ------------------------------//

	/**
	 * @brief remove unwanted leading/trailing characters from output of serializing element tree
	 * taking this manual step at the end allows the main algorithm to be clean and simple
	 */
	string removeLeadingTrailing(string input) {
		return input.substr(4, input.length()-4);
	}

	/**
	 * @brief when a branch ends, traverese backwards up the tree to find the next element to serialize, appending a closing bracket each time
	 * this method is neccessary to handle multiple closing brackets in a row
	 */
	Element* exitBranchAppend(Element* pElement, string &output) {
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
	 * @brief serialize the element tree to output a json string
	 */
	string generateJsonString () {
		string output;
		Element* pElement = m_pFirstElement;
		if (isPrimitiveJson()) {
			return m_pFirstElement->getValue();
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
				output.append(currentElement.getKey() + currentElement.getValue());
				pElement = exitBranchAppend(pElement, output);
			} else {
				break;
			}
		}
		output = removeLeadingTrailing(output);
		return output;
	}
public:
	string serialize() {
		return generateJsonString();
	}

	//----------------------------- GET METHODS ------------------------------//
private:
	/**
	 * @brief search the top layer of the element tree for a given key then return that element
	 */
	Element* getElement(string key) {
		Element* pElement = m_pFirstElement->m_pChildElement;
		while(pElement) {
			if (pElement->m_key == key) return pElement;
			pElement = pElement->getNext();
		}
		return nullptr;
	}

	/**
	 * @brief return the element at a given index in the top layer of the element tree
	 */
	Element* getElement(int index) {
		Element* pElement = m_pFirstElement->m_pChildElement;
		int current = 0;
		while(pElement) {
			if (current == index) return pElement;
			pElement = pElement->getNext();
			current++;
		}
		return nullptr;
	}
public:
	/**
	 * @brief get json value by key. Search the top layer then implicitly construct a new SimpleJson with the found element as its first element
	 * implicit construction is needed as otherwise we would create a SimpleJson in this scope then copy it, resulting in dangling ptrs
	*/
	SimpleJson get (string key) {
		if (m_pFirstElement->m_valueType == Element::valueType::ARRAY) throw invalid_argument("cannot get an array by key");
		Element* firstElement = getElement(key);
		if (!firstElement) return NULL;
		return firstElement;
	}

	/**
	 * @brief get json value by index. Search the top layer then implicitly construct a new SimpleJson with the found element as its first element
	 * implicit construction is needed as otherwise we would create a SimpleJson in this scope then copy it, resulting in dangling ptrs
	*/
	SimpleJson get (int index) {
		if (m_pFirstElement->m_valueType == Element::valueType::OBJECT) throw invalid_argument("cannot get an object by index");
		Element* firstElement = getElement(index);
		if (!firstElement) return NULL;
		return firstElement;
	}

	/**
	 * @brief check if the SimpleJson object is a bool
	 */
	bool isBool() {
		return m_pFirstElement->m_valueType == Element::valueType::BOOL;
	}

	/**
	 * @brief return the value from a SimpleJson that contains just one element of type bool
	 */
	bool getBool() {
		if(m_pFirstElement->m_valueType != Element::valueType::BOOL) throw invalid_argument("element is not a bool");
		return m_pFirstElement->m_value == "true";
	}

	/**
	 * @brief check if the SimpleJson object is a string
	 */
	bool isString() {
		return m_pFirstElement->m_valueType == Element::valueType::STRING;
	}

	/**
	 * @brief return the value from a SimpleJson that contains just one element of type string
	 */
	string getString() {
		if(m_pFirstElement->m_valueType != Element::valueType::STRING) throw invalid_argument("element is not a string");
		return m_pFirstElement->getValue();
	}

	/**
	 * @brief check if the SimpleJson object is a number
	 */
	bool isFloat() {
		return m_pFirstElement->m_valueType == Element::valueType::NUMBER;
	}

	/**
	 * @brief return the value from a SimpleJson that contains just one element of type number
	 */
	float getFloat() {
		if(m_pFirstElement->m_valueType != Element::valueType::NUMBER) throw invalid_argument("element is not a number");
		return stof(m_pFirstElement->getValue());
	}

	//----------------------------- SET METHODS ------------------------------//
private:
	/**
	 * @brief lookup the element to be set specified by its key - if not found add a new element
	 * The branch to be searched is determined by the starting element passed in. This is needed so the user can set values more than one layer deep in the tree
	 */
	Element* findOrAddElement(Element* startingElement, string key) {
		Element* pElement = m_pFirstElement->getChild();
		if (startingElement) pElement = startingElement->getChild();
		while(pElement) {
			if (pElement->m_key == key) return pElement;
			if (pElement->getNext()) {
				pElement = pElement->getNext();
			} else {
				pElement = addElement(pElement);
				pElement->setKey(key);
				return pElement;
			}
		}
		return nullptr;
	}

	/**
	 * @brief lookup the element to be set specified by its index - if not found add null elements until the index is reached
	 * The branch to be searched is determined by the starting element passed in. This is needed so the user can set values more than one layer deep in the tree
	 */
	Element* findOrAddElement(Element* startingElement, int index) {
		Element* pElement = m_pFirstElement->getChild();
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
				pElement->setValue("null");
			}
			current++;
		}
		return nullptr;
	}

	/**
	 * @class Proxy
	 * temporary object used to store a pointer to the element reutrned from each call to key()
	 * this is needed so that key() can be called multiple times in a row to set a value more than 1 layer deep in the tree
	 * it is temporary so that preivious calls to key().set() do not effect later calls
	 */
	class Proxy
	{
	private:
		SimpleJson& m_json;
		Element* m_element = nullptr;
	public:
		Proxy(SimpleJson& json) : m_json(json) {}

		/**
		 * @brief search from the starting element for an element with a given key
		 */
		Proxy& key(string key) {
			m_element = m_json.findOrAddElement(m_element, key);
			if (!m_element) throw invalid_argument("could not find or create this key");
			return *this;
		}

		/**
		 * @brief search from the starting element for an element with a given index
		 */
		Proxy& key(int index) {
			m_element = m_json.findOrAddElement(m_element, index);
			if (!m_element) throw invalid_argument("could not find or create this index");
			return *this;
		}

		/**
		 * @brief expose Element::setBool so it can be called straight after a call to key()
		 */
		void setBool(bool value) {
			m_element->setBool(value);
		}

		/**
		 * @brief expose Element::setString so it can be called straight after a call to key()
		 */
		void setString(string value) {
			m_element->setString(value);
		}

		/**
		 * @brief expose Element::setFloat so it can be called straight after a call to key()
		 */
		void setFloat(float value) {
			m_element->setFloat(value);
		}
	};
private:
	list<Proxy*> m_pProxys;
public:
	/**
	 * @brief find by key the element whose value should be set in the subsequent call to set()
	 * returns a temporary proxy object which stores a pointer to the value to be set
	*/
	Proxy key (string key) {
		Proxy* pProxy = new Proxy(*this);
		m_pProxys.push_back(pProxy);
		return pProxy->key(key);
	}

	/**
	 * @brief find by index the element whose value should be set in the subsequent call to set()
	 * returns a temporary proxy object which stores a pointer to the value to be set
	*/
	Proxy key(int index) {
		Proxy* pProxy = new Proxy(*this);
		m_pProxys.push_back(pProxy);
		return pProxy->key(index);
	}
};