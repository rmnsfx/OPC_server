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

	char readBuffer[65536];

	FILE* fp = fopen(path, "r");
	FileReadStream is(fp, readBuffer, sizeof(readBuffer));

	Document doc;
	doc.ParseStream(is);
	fclose(fp);

	printf("Parsing json complete.\n\n");

	const Value& Coms = doc["Coms"];
	const Value& Tags = doc["Tags"];


	if (doc.IsObject() == true)
	{
		
		controller.name = doc["name"].GetString();
		controller.type = doc["type"].GetUint();
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
				node.type = Coms[i]["type"].GetUint();
				node.intertype = Coms[i]["intertype"].GetString();
				node.address = Coms[i]["address"].GetString();
				node.port = Coms[i]["port"].GetUint();	
				node.baud_rate = Coms[i]["baud_rate"].GetUint();
				node.word_lenght = Coms[i]["word_lenght"].GetUint();
				node.parity = Coms[i]["parity"].GetUint();
				node.stop_bit = Coms[i]["stop_bit"].GetUint();
				node.description = Coms[i]["comment"].GetString();
				node.attribute = Coms[i]["attribute"].GetUint();
				node.poll_period = Coms[i]["period"].GetUint();


				if (Coms[i]["Devs"].Size() > 0)
				{
					node.vectorDevice.clear();

					for (int j = 0; j < Coms[i]["Devs"].Size(); j++)
					{
						printf("         Devs: %s\n", Coms[i]["Devs"][j]["name"].GetString());						

						device.name = Coms[i]["Devs"][j]["name"].GetString();
						device.on = Coms[i]["Devs"][j]["on"].GetUint();						
						device.type = Coms[i]["Devs"][j]["type"].GetUint();
						device.devtype = Coms[i]["Devs"][j]["devtype"].GetUint();						
						device.device_address = Coms[i]["Devs"][j]["address_device"].GetUint();
						device.description = Coms[i]["Devs"][j]["comment"].GetString();												
						device.poll_timeout = Coms[i]["Devs"][j]["timeout"].GetUint();
						device.attribute = Coms[i]["Devs"][j]["attribute"].GetUint();

						if (Coms[i]["Devs"][j]["Tags"].Size() > 0)
						{
							device.vectorTag.clear();

							for (int k = 0; k < Coms[i]["Devs"][j]["Tags"].Size(); k++)
							{
								printf("            Tag: %s\n", Coms[i]["Devs"][j]["Tags"][k]["name"].GetString());

								tags.name = Coms[i]["Devs"][j]["Tags"][k]["name"].GetString();
								tags.on = Coms[i]["Devs"][j]["Tags"][k]["on"].GetUint();								
								tags.data_type = Coms[i]["Devs"][j]["Tags"][k]["data_type"].GetString();

								tags.enum_data_type = type_converter( Coms[i]["Devs"][j]["Tags"][k]["data_type"].GetString() );

								tags.function = Coms[i]["Devs"][j]["Tags"][k]["function"].GetUint();
								tags.reg_address = Coms[i]["Devs"][j]["Tags"][k]["register"].GetUint(); //Адрес регистра = Номер регистра - 1
								tags.coef_A = Coms[i]["Devs"][j]["Tags"][k]["coef_A"].GetUint();
								tags.coef_B = Coms[i]["Devs"][j]["Tags"][k]["coef_B"].GetUint();
								
								tags.description = Coms[i]["Devs"][j]["Tags"][k]["comment"].GetString();
								tags.attribute = Coms[i]["Devs"][j]["Tags"][k]["attribute"].GetUint();

								device.vectorTag.push_back(tags);
							}
						}
						else device.vectorTag.clear();

						node.vectorDevice.push_back(device);
					}
				}
				else node.vectorDevice.clear();





				//if (Coms[i]["Tags"].Size() > 0)
				//{

				//	for (int j = 0; j < Coms[i]["Tags"].Size(); j++)
				//	{
				//		//printf("Tags: %s\n", Tags[i]["Tags"][j]["name"].GetString());

				//		nodeTags.name = Tags[i]["Tags"][j]["name"].GetString();
				//		nodeTags.type = Tags[i]["Tags"][j]["type"].GetUint();
				//		nodeTags.tagtype = Tags[i]["Tags"][j]["tagtype"].GetString();
				//		nodeTags.string = Tags[i]["Tags"][j]["string"].GetString();
				//		nodeTags.address = Tags[i]["Tags"][j]["address"].GetString();
				//		nodeTags.description = Tags[i]["Tags"][j]["comment"].GetString();
				//		nodeTags.attribute = Tags[i]["Tags"][j]["attribute"].GetUint();

				//		node.vectorNodeTag.push_back(nodeTags);
				//	}
				//}
				//else node.vectorNodeTag.clear();

				controller.vectorNode.push_back(node);

			} //ForLoop Coms (Node) end
		} //If Coms(Node) end


		//if (Tags.Size() > 0)
		//{

		//	for (int i = 0; i < Tags.Size(); i++)
		//	{
		//		//printf("Tags: %s\n", Tags[i]["name"].GetString());

		//		controllerTags.name = Tags[i]["name"].GetString();
		//		controllerTags.type = Tags[i]["type"].GetUint();
		//		controllerTags.tagtype = Tags[i]["tagtype"].GetString();
		//		controllerTags.string = Tags[i]["string"].GetString();
		//		controllerTags.address = Tags[i]["address"].GetString();
		//		controllerTags.description = Tags[i]["comment"].GetString();
		//		controllerTags.attribute = Tags[i]["attribute"].GetUint();
		//	}

		//	controller.vectorControllerTag.push_back(controllerTags);
		//}
		//else controller.vectorControllerTag.clear();

	} //Server/Controller end

	return controller;
}