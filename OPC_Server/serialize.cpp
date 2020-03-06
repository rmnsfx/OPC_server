#include <cstdio>
#include <cstdlib>
#include <cstdio>
#include <cstdlib>
#include <signal.h>
#include <pthread.h>
#include <string> 
#include <iostream>
#include <vector>
#include <unistd.h>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filereadstream.h"

#include "serialize.h"

using namespace rapidjson;

Controller serializeFromJSON(char* path)
{

	Controller controller;
	Node node;
	Device device;
	Tag tags;
	Tag nodeTags;
	Tag controllerTags;
	Document doc;

	//char* readBuffer = (char*) calloc(65536, sizeof(char)); // Выделяем память 	
	char* readBuffer = new char[65536];
	FILE* fp = fopen(path, "r");

	
	if (fp == 0x00)
	{
		printf("!!! Not found opc.JSON. Exit.\n");
		
		exit(1);
	}

	
	FileReadStream is(fp, readBuffer, sizeof(readBuffer));
	doc.ParseStream(is);
	fclose(fp);

	printf("Read json complete.\n\n");

	const Value& Coms = doc["children"];
	

	if (doc.IsObject() == true)
	{
		
		controller.name = doc["name"].GetString();
		//controller.type = doc["type"].GetString();
		controller.description = doc["comment"].GetString();
		controller.attribute = doc["attribute"].GetUint();

		printf("%s:\n", doc["name"].GetString());

		if (Coms.Size() > 0)
		{

			controller.vectorNode.clear();

			for (int i = 0; i < Coms.Size(); i++)
			{
				printf("    Coms: %s\n", Coms[i]["name"].GetString());				
								
				node.name = Coms[i]["name"].GetString();
				node.on = Coms[i]["on"].GetUint();
				//node.type = Coms[i]["type"].GetString();
				//node.intertype = Coms[i]["intertype"].GetString();
				node.enum_interface_type = interface_converter( Coms[i]["intertype"].GetString() );
				//node.address = Coms[i]["address"].GetString();
				node.port = Coms[i]["port"].GetUint();	
				node.baud_rate = 3000000; //Coms[i]["baud_rate"].GetUint();
				node.word_lenght = 8;//node.word_lenght = Coms[i]["word_lenght"].GetUint();
				node.parity = 0;//node.parity = Coms[i]["parity"].GetUint();
				node.stop_bit = 1;//node.stop_bit = Coms[i]["stop_bit"].GetUint();
				node.description = Coms[i]["comment"].GetString();
				node.attribute = Coms[i]["attribute"].GetUint();
				node.poll_period = Coms[i]["period"].GetUint();


				if (Coms[i]["children"].Size() > 0)
				{
					node.vectorDevice.clear();

					for (int j = 0; j < Coms[i]["children"].Size(); j++)
					{
						printf("         Devs: %s\n", Coms[i]["children"][j]["name"].GetString());						

						device.name = Coms[i]["children"][j]["name"].GetString();
						device.on = Coms[i]["children"][j]["on"].GetUint();						
						//device.type = Coms[i]["Devs"][j]["type"].GetString();
						device.devtype = Coms[i]["children"][j]["devtype"].GetUint();						
						device.device_address = Coms[i]["children"][j]["address_device"].GetUint();
						device.description = Coms[i]["children"][j]["comment"].GetString();												
						device.poll_timeout = Coms[i]["children"][j]["timeout"].GetUint();
						device.attribute = Coms[i]["children"][j]["attribute"].GetUint();

						if (Coms[i]["children"][j]["children"].Size() > 0)
						{
							device.vectorTag.clear();

							for (int k = 0; k < Coms[i]["children"][j]["children"].Size(); k++)
							{
								printf("            Tag: %s\n", Coms[i]["children"][j]["children"][k]["name"].GetString());

								tags.name = Coms[i]["children"][j]["children"][k]["name"].GetString();
								tags.on = Coms[i]["children"][j]["children"][k]["on"].GetUint();																
								tags.enum_data_type = type_converter( Coms[i]["children"][j]["children"][k]["data_type"].GetString() ); //Конвертируем типы в enum

								
								tags.function = Coms[i]["children"][j]["children"][k]["function"].GetUint();
								tags.reg_address = Coms[i]["children"][j]["children"][k]["register"].GetUint(); 
								tags.coef_A = Coms[i]["children"][j]["children"][k]["coef_A"].GetFloat();
								tags.coef_B = Coms[i]["children"][j]["children"][k]["coef_B"].GetFloat();
								
								tags.description = Coms[i]["children"][j]["children"][k]["comment"].GetString();
								tags.attribute = Coms[i]["children"][j]["children"][k]["attribute"].GetUint();

								device.vectorTag.push_back(tags);
							}
						}
						else device.vectorTag.clear();

						node.vectorDevice.push_back(device);
					}
				}
				else node.vectorDevice.clear();


				controller.vectorNode.push_back(node);

			} //ForLoop Coms (Node) end

		} //If Coms(Node) end

	} //Server/Controller end


	delete []readBuffer;


	return controller;
}