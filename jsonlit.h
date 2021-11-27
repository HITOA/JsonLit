#pragma once
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <algorithm>
#include <any>
#include <stdexcept>

#pragma region Tokenizer

enum TokenType {
	UNKNOWN,
	STRING,
	NUMBER,
	BOOL,
	NONE,
	OPERATOR
};

struct Token {
	TokenType type;
	std::string value;
};

std::pair<int, Token> TokenizeString(const std::string& src, int pos) {
	std::pair<int, Token> r{};

	int lenght = 0;

	if (src[pos] == '"') {
		pos++;
		do {
			lenght++;
			pos++;
		} while (pos < src.size() && src[pos] != '"');

		if (src[pos] != '"')
			return r;
	}
	else {
		return r;
	}

	r.first = lenght + 2;
	r.second.type = TokenType::STRING;
	r.second.value = src.substr(pos - lenght, lenght);

	return r;
}

std::pair<int, Token> TokenizeNumber(const std::string& src, int pos) {
	std::pair<int, Token> r{};
	int length = 0;

	while (isdigit(src[pos])) {
		length++;
		pos++;

		if (pos >= src.size())
			break;
	}

	r.first = length;
	r.second.type = TokenType::NUMBER;
	r.second.value = src.substr(pos - length, length);

	return r;
}

std::pair<int, Token> TokenizeBool(const std::string& src, int pos) {
	std::pair<int, Token> r{};

	if (src.substr(pos, 4) == "true") {
		r.first = 4;
		r.second.type = TokenType::BOOL;
		r.second.value = src.substr(pos, 4);
	}
	else if (src.substr(pos, 5) == "false") {
		r.first = 5;
		r.second.type = TokenType::BOOL;
		r.second.value = src.substr(pos, 5);
	}

	return r;
}

std::pair<int, Token> TokenizeNull(const std::string& src, int pos) {
	std::pair<int, Token> r{};

	if (src.substr(pos, 4) == "null") {
		r.first = 4;
		r.second.type = TokenType::NONE;
		r.second.value = src.substr(pos, 4);
	}

	return r;
}

std::pair<int, Token> TokenizeOperator(const std::string& src, int pos) {
	std::vector<char> operators{ '{', '}', '[', ']', ':', ',' };
	std::pair<int, Token> r{};

	if (std::find(operators.begin(), operators.end(), src[pos]) != operators.end()) {
		r.first++;
		r.second.type = TokenType::OPERATOR;
		r.second.value = src[pos];
	}

	return r;
}

std::pair<int, Token> Tokenize(const std::string& src, int pos) {
	std::vector<std::function<std::pair<int, Token>(const std::string&, int)>> tokenizers
	{
		TokenizeString,
		TokenizeNumber,
		TokenizeBool,
		TokenizeNull,
		TokenizeOperator
	};

	std::pair<int, Token> r{};

	for (const auto& tokenizer : tokenizers) {
		r = tokenizer(src, pos);
		if (r.first > 0)
			break;
	}
	
	if (r.first == 0) {
		r.first++;
		r.second.type = TokenType::UNKNOWN;
		r.second.value = src[pos];
	}

	return r;
}

std::vector<std::pair<int, Token>> TokenizeAll(const std::string& src) {
	std::vector<std::pair<int, Token>> tokens{};

	int pos = 0;

	do {
		while (src[pos] == ' ' || 
				src[pos] == '\n' || 
				src[pos] == '	')
			pos++;

		std::pair<int, Token> current = Tokenize(src, pos);
		tokens.push_back(current);
		pos += current.first;
	} while (pos < src.size());

	return tokens;
}

#pragma endregion

#pragma region Parser

enum class JsonObjectType {
	Unknown,
	Integer,
	String,
	Boolean,
	Object,
	Array
};

struct JsonObject {
public:
	JsonObject& operator[] (std::size_t idx) {
		if (GetType() == JsonObjectType::Array)
			return std::any_cast<std::vector<JsonObject>>(value)[idx];
		throw;
	}

	JsonObject& operator= (JsonObject& jsonObject) {
		value = jsonObject.value;
		return *this;
	}

	JsonObject& operator= (int integer) {
		value = integer;
		return *this;
	}

	JsonObject& operator= (const std::string& string) {
		value = string;
		return *this;
	}

	JsonObject& operator= (const char* string) {
		value = std::string{ string };
		return *this;
	}

	JsonObject& operator= (bool boolean) {
		value = boolean;
		return *this;
	}

	JsonObject& operator= (const std::map<std::string, JsonObject>& object) {
		value = object;
		return *this;
	}

	JsonObject& operator= (const std::vector<JsonObject>& jsonArray) {
		value = jsonArray;
		return *this;
	}
public:
	int ToInteger() {
		if (GetType() == JsonObjectType::Integer)
			return std::any_cast<int>(value);
		throw;
	}

	std::string ToString() {
		if (GetType() == JsonObjectType::String)
			return std::any_cast<std::string>(value);
		throw;
	}

	bool ToBoolean() {
		if (GetType() == JsonObjectType::Boolean)
			return std::any_cast<bool>(value);
		throw;
	}

	std::map<std::string, JsonObject> ToObject() {
		if (GetType() == JsonObjectType::Object)
			return std::any_cast<std::map<std::string, JsonObject>>(value);
		throw;
	}

	std::vector<JsonObject> ToArray() {
		if (GetType() == JsonObjectType::Array)
			return std::any_cast<std::vector<JsonObject>>(value);
		throw;
	}

	JsonObjectType GetType() {
		if (value.type() == typeid(int)) {
			return JsonObjectType::Integer;
		}
		if (value.type() == typeid(std::string)) {
			return JsonObjectType::String;
		}
		if (value.type() == typeid(bool)) {
			return JsonObjectType::Boolean;
		}
		if (value.type() == typeid(std::map<std::string, JsonObject>)) {
			return JsonObjectType::Object;
		}
		if (value.type() == typeid(std::vector<JsonObject>)) {
			return JsonObjectType::Array;
		}
		return JsonObjectType::Unknown;
	}
private:
	std::any value;
};

std::pair<int, JsonObject> ParseJsonObject(std::vector<std::pair<int, Token>> tokens, int position) {
	JsonObject r{};
	int initialPos = position;

	if (tokens[position].second.type == TokenType::STRING) {
		r = tokens[position].second.value;
		return std::make_pair(1, r);
	}

	if (tokens[position].second.type == TokenType::NUMBER) {
		r = std::stoi(tokens[position].second.value);
		return std::make_pair(1, r);
	}

	if (tokens[position].second.type == TokenType::OPERATOR && tokens[position].second.value == "{") {
		//is an json object
		std::map<std::string, JsonObject> jsonObject;

		while (tokens[position].second.value != "}") 
		{
			position++;

			if (tokens[position].second.type != TokenType::STRING)
				throw std::runtime_error("bad format.");

			std::string currentName = tokens[position].second.value;
			position++;

			if (tokens[position].second.type != TokenType::OPERATOR)
				throw std::runtime_error("bad format");

			position++;

			std::pair<int, JsonObject> currentObject = ParseJsonObject(tokens, position);
			jsonObject.insert(std::make_pair(currentName, currentObject.second));

			position += currentObject.first;

			if (position >= tokens.size())
				throw std::runtime_error("\"}\" missing.");
		}

		r = jsonObject;
		return std::make_pair(position - initialPos + 1, r);
	}

	if (tokens[position].second.type == TokenType::OPERATOR && tokens[position].second.value == "[") {

		std::vector<JsonObject> jsonArray{};

		while (tokens[position].second.value != "]")
		{
			position++;

			std::pair<int, JsonObject> jsonObject = ParseJsonObject(tokens, position);

			position += jsonObject.first;
			jsonArray.push_back(jsonObject.second);
			
			if (position >= tokens.size())
				throw std::runtime_error("\"]\" missing.");
		}

		r = jsonArray;
		return std::make_pair(position - initialPos + 1, r);
	}

	throw std::runtime_error("bad format.");
}

JsonObject Parse(std::vector<std::pair<int, Token>> tokens) {
	std::pair<int, JsonObject> jsonObject = ParseJsonObject(tokens, 0);

	return jsonObject.second;
}

#pragma endregion

JsonObject ParseJsonString(const std::string& src) {
	return Parse(TokenizeAll(src));
}