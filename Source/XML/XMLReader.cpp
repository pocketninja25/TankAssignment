#include "XMLReader.h"

#include "EntityManager.h"

namespace gen {

extern CEntityManager EntityManager;

CEntityTemplate* XMLReader::LoadEntityTemplate(const string& fileName)
{
	string file = filePath + fileName;

	TiXmlDocument doc = TiXmlDocument(file.c_str());
	bool loadSuccess = doc.LoadFile();

	if (loadSuccess)
	{
		//Define data types to extract
		string type, name, mesh;

		TiXmlElement* rootElement = doc.RootElement();

		if (rootElement)
		{
			type = rootElement->Attribute("Type");
			name = rootElement->Attribute("Name");
			mesh = rootElement->Attribute("Mesh");
			return EntityManager.CreateTemplate(type, name, mesh);
		}
	}
	return nullptr;
}

CTankTemplate* XMLReader::LoadTankTemplate(const string& fileName)
{
	string file = filePath + fileName;

	TiXmlDocument doc = TiXmlDocument(file.c_str());
	bool loadSuccess = doc.LoadFile();

	if (loadSuccess)
	{
		//Define data types to extract
		string type, name, mesh;
		float maxSpeed, acceleration, turnSpeed, turretturnSpeed, shellSpeed, shellLifetime, radius;
		int maxHP, shellDamage;

		TiXmlElement* rootElement = doc.RootElement();
		if (rootElement)
		{
			type = rootElement->Attribute("Type");
			name = rootElement->Attribute("Name");
			mesh = rootElement->Attribute("Mesh");
			
			maxSpeed =			static_cast<float>(atof(rootElement->Attribute("MaxSpeed")));
			acceleration =		static_cast<float>(atof(rootElement->Attribute("Acceleration")));
			turnSpeed =			static_cast<float>(atof(rootElement->Attribute("TurnSpeed")));
			turretturnSpeed =	static_cast<float>(atof(rootElement->Attribute("TurretTurnSpeed")));
			shellSpeed =		static_cast<float>(atof(rootElement->Attribute("ShellSpeed")));
			shellLifetime =		static_cast<float>(atof(rootElement->Attribute("ShellLifetime")));
			radius =			static_cast<float>(atof(rootElement->Attribute("Radius")));

			maxHP =			atoi(rootElement->Attribute("MaxHP"));
			shellDamage =	atoi(rootElement->Attribute("ShellDamage"));
		
			return EntityManager.CreateTankTemplate(type, name, mesh, maxSpeed, acceleration, turnSpeed, turretturnSpeed, maxHP, shellDamage, shellSpeed, shellLifetime, radius);
		}	
	}
	return nullptr;
}

void XMLReader::LoadScene(const string & filename)
{
	string file = filePath + filename;

	TiXmlDocument doc = TiXmlDocument(file.c_str());
	bool loadSuccess = doc.LoadFile();

	if (loadSuccess)
	{
		TiXmlElement* rootElement = doc.RootElement();

		if (rootElement)
		{
			//Traverse the templates list
			TiXmlElement* templatesElt = rootElement->FirstChildElement("Templates");

			//Search for entity templates
			TiXmlElement* traversalElt = templatesElt->FirstChildElement("EntityTemplate");
			while (traversalElt)
			{
				//Create the found entity template
				EntityManager.CreateTemplate(traversalElt->Attribute("file"));

				//Move to the next entity template
				traversalElt = traversalElt->NextSiblingElement("EntityTemplate");
			}
			//Search for tank templates
			traversalElt = templatesElt->FirstChildElement("TankTemplate");
			while (traversalElt)
			{
				//Create the found tank template
				EntityManager.CreateTankTemplate(traversalElt->Attribute("file"));

				//Move to the next tank template
				traversalElt = traversalElt->NextSiblingElement("TankTemplate");
			}

			//Traverse the entities list
			TiXmlElement* entitiesElt = rootElement->FirstChildElement("Entities");
			traversalElt = entitiesElt->FirstChildElement("Entity");
			while (traversalElt)
			{
				string templateName = traversalElt->Attribute("templateName");
				string name = traversalElt->Attribute("name");
				CVector3 position, rotation, scale;

				//Get entity position, rotation and scale values
				TiXmlElement* vector3Elt = traversalElt->FirstChildElement("Position");	//Get position
				if (vector3Elt)
				{
					//The element exists, populate based on its data
					position.x = static_cast<float>(atof(vector3Elt->Attribute("x")));
					position.y = static_cast<float>(atof(vector3Elt->Attribute("y")));
					position.z = static_cast<float>(atof(vector3Elt->Attribute("z")));
				}
				else
				{
					//The element did not exist, use default value
					position = CVector3::kOrigin;
				}

				vector3Elt = traversalElt->FirstChildElement("Rotation");	//Get rotation
				if (vector3Elt)
				{
					//The element exists, populate based on its data
					rotation.x = static_cast<float>(atof(vector3Elt->Attribute("x")));
					rotation.y = static_cast<float>(atof(vector3Elt->Attribute("y")));
					rotation.z = static_cast<float>(atof(vector3Elt->Attribute("z")));
				}
				else
				{
					//The element did not exist, use default value
					rotation = CVector3::kZero;
				}

				vector3Elt = traversalElt->FirstChildElement("Scale");	//Get scale
				if (vector3Elt)
				{
					//The element exists, populate based on its data
					scale.x = static_cast<float>(atof(vector3Elt->Attribute("x")));
					scale.y = static_cast<float>(atof(vector3Elt->Attribute("y")));
					scale.z = static_cast<float>(atof(vector3Elt->Attribute("z")));
				}
				else
				{
					//The element did not exist, use default value
					scale = CVector3::kOne;
				}

				EntityManager.CreateEntity(templateName, name, position, rotation, scale);

				traversalElt = traversalElt->NextSiblingElement("Entity");
			}

			//Traverse the tank list
			TiXmlElement* tankElt = rootElement->FirstChildElement("Tanks");
			traversalElt = tankElt->FirstChildElement("Tank");
			while (traversalElt)
			{
				string templateName = traversalElt->Attribute("templateName");
				string name = traversalElt->Attribute("name");
				int team = atoi(traversalElt->Attribute("team"));
				vector<CVector3> patrolRoute = LoadPatrolRoute(traversalElt->Attribute("patrolRoute"));
				CVector3 position, rotation, scale;

				//Get entity position, rotation and scale values
				TiXmlElement* vector3Elt = traversalElt->FirstChildElement("Position");	//Get position
				if (vector3Elt)
				{
					//The element exists, populate based on its data
					position.x = static_cast<float>(atof(vector3Elt->Attribute("x")));
					position.y = static_cast<float>(atof(vector3Elt->Attribute("y")));
					position.z = static_cast<float>(atof(vector3Elt->Attribute("z")));
				}
				else
				{
					//The element did not exist, use default value
					position = CVector3::kOrigin;
				}

				vector3Elt = traversalElt->FirstChildElement("Rotation");	//Get rotation
				if (vector3Elt)
				{
					//The element exists, populate based on its data
					rotation.x = static_cast<float>(atof(vector3Elt->Attribute("x")));
					rotation.y = static_cast<float>(atof(vector3Elt->Attribute("y")));
					rotation.z = static_cast<float>(atof(vector3Elt->Attribute("z")));
				}
				else
				{
					//The element did not exist, use default value
					rotation = CVector3::kZero;
				}

				vector3Elt = traversalElt->FirstChildElement("Scale");	//Get scale
				if (vector3Elt)
				{
					//The element exists, populate based on its data
					scale.x = static_cast<float>(atof(vector3Elt->Attribute("x")));
					scale.y = static_cast<float>(atof(vector3Elt->Attribute("y")));
					scale.z = static_cast<float>(atof(vector3Elt->Attribute("z")));
				}
				else
				{
					//The element did not exist, use default value
					scale = CVector3::kOne;
				}

				EntityManager.CreateTank(templateName, team, patrolRoute, name, position, rotation, scale);

				traversalElt = traversalElt->NextSiblingElement("Tank");
			}
		}
	}
}

vector<CVector3> XMLReader::LoadPatrolRoute(const string & filename)
{
	string file = filePath + filename;
	vector<CVector3> patrolRoute;

	TiXmlDocument doc = TiXmlDocument(file.c_str());
	bool loadSuccess = doc.LoadFile();

	if (loadSuccess)
	{
		TiXmlElement* rootElement = doc.RootElement();
		if (rootElement)
		{
			TiXmlElement* traversalElt = rootElement->FirstChildElement("Waypoint");
			while (traversalElt)
			{
				CVector3 waypoint;

				waypoint.x = static_cast<float>(atof(traversalElt->Attribute("x")));
				waypoint.y = static_cast<float>(atof(traversalElt->Attribute("y")));
				waypoint.z = static_cast<float>(atof(traversalElt->Attribute("z")));

				patrolRoute.push_back(waypoint);
				traversalElt = traversalElt->NextSiblingElement("Waypoint");
			}
		}
	}

	return patrolRoute;
}

//vector<string> XMLReader::LoadEntityTemplateList(const string & filename)
//{
//	string file = filePath + filename;
//	vector<string> templateList;
//
//	TiXmlDocument doc = TiXmlDocument(file.c_str());
//	bool loadSuccess = doc.LoadFile();
//
//	if (loadSuccess)
//	{
//		TiXmlElement* rootElement = doc.RootElement();
//
//		if (rootElement)
//		{
//			rootElement = rootElement->FirstChildElement("Templates");
//			TiXmlElement* traversalElt = rootElement->FirstChildElement("EntityTemplate");
//			while (traversalElt)
//			{
//				templateList.push_back(traversalElt->Attribute("file"));
//				
//				traversalElt = traversalElt->NextSiblingElement("EntityTemplate");
//			}
//		}
//	}
//	return templateList;
//}
//
//vector<string> XMLReader::LoadTankTemplateList(const string & filename)
//{
//	string file = filePath + filename;
//	vector<string> templateList;
//
//	TiXmlDocument doc = TiXmlDocument(file.c_str());
//	bool loadSuccess = doc.LoadFile();
//
//	if (loadSuccess)
//	{
//		TiXmlElement* rootElement = doc.RootElement();
//
//		if (rootElement)
//		{
//			rootElement = rootElement->FirstChildElement("Templates");
//			TiXmlElement* traversalElt = rootElement->FirstChildElement("TankTemplate");
//			while (traversalElt)
//			{
//				templateList.push_back(traversalElt->Attribute("file"));
//
//				traversalElt = traversalElt->NextSiblingElement("TankTemplate");
//			}
//		}
//	}
//	return templateList;
//}
//


}