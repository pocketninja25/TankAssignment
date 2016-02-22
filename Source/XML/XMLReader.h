#pragma once

#include "tinyxml.h"
#include <string>

#include "Entity.h"
#include "TankEntity.h"
#include "ShellEntity.h"

namespace gen{

class XMLReader
{
public:
	XMLReader(string path = "")
	{
		filePath = path;
	}
	~XMLReader()
	{
	}

	CEntityTemplate* LoadEntityTemplate(const string& filename);

	CTankTemplate* LoadTankTemplate(const string& filename);
	
	void LoadScene(const string& filename);

	vector<CVector3> LoadPatrolRoute(const string& filename);

	//vector<string> LoadEntityTemplateList(const string& filename);
	//
	//vector<string> LoadTankTemplateList(const string& filename);

	void SetFilePath(const string& path)
	{
		filePath = path;
	}

private:
	string filePath;
};

}
