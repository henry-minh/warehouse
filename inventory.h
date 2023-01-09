
#ifndef INVENTORY_H_
#define INVENTORY_H_


#include <vector>
#include <string>
#include <fstream>
#include "json/jsoncpp.cpp"

using namespace std;

class inventory
{
	public:
		class product{
			public:
				product(int,string);
				int amount;
				string prodName;

		};

		inventory(bool);
		vector<product*> inv;

	private:
		string filename = "products.json";

};

inventory::product::product(int a, string p){
	amount = a;
	prodName = p;
}

inventory::inventory(bool allocInv){
	//read in json file
	ifstream file("/tmp/inventory.json");
	Json::Reader reader;
	Json::Value values;

	reader.parse(file,values);

	auto prod = values["products"];
	int size = prod.size();

	int tempAmount;

	for(int i =0; i < size; i++){
		if(allocInv == true){
			 tempAmount = 0;
		} else{
			tempAmount = prod[i]["amount"].asInt();
		}

		string tempProdName = prod[i]["product"].asString();
		product* tempProd = new product(tempAmount,tempProdName);
		inv.push_back(tempProd);
	}
}



#endif /* INVENTORY_H_ */
