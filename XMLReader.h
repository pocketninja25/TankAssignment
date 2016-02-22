#pragma once

#include ""


class XMLReader
{
public:
	XMLReader();
	~XMLReader();

private:
	XML_Parser m_Parser;
};

