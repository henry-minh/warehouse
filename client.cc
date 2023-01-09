#include <iostream>
#include <fstream>
#include <stdio.h>
#include "json/jsoncpp.cpp"
#include <sys/neutrino.h>
#include <sys/dispatch.h>
#include <sys/iofunc.h>
#include "order.h"
using namespace std;



int main() {
	//read json file and parse values using jsoncpp library
	ifstream file("/tmp/orders1000.json");
	Json::Reader reader;
	Json::Value values;
	reader.parse(file, values);

	auto orders = values["orders"];
	int size = orders.size();
	order* orderList = new order[size];

	//populate orderList structures
	for (int i = 0; i < size; i++){
		orderList[i].orderNo = i+1;
		orderList[i].customer = orders[i]["customer"].asString();
		orderList[i].price = orders[i]["price"].asDouble();
		orderList[i].prime = orders[i]["prime"].asBool();

		auto orderItems = orders[i]["item"];

		for (int j = 0; j < orderItems.size(); j++) {
			orderList[i].item[j].item = orderItems[j]["item"].asString();
			orderList[i].item[j].quantity = orderItems[j]["quantity"].asInt();
		}
	}

	int coid = name_open("BCHT_Warehouse_Channel", 0);

	//loop for sending orders to the server
	for (int i = 0; i < size; i++) {
		int order_status = -1;
		MsgSend(coid, &orderList[i], sizeof(orderList[i]), &order_status, sizeof(order_status));

		if (order_status == 1) {
			cout << "Order " << orderList[i].orderNo << " approved." << endl;
		} else {
			cout << "Order " << orderList[i].orderNo << " rejected. Not enough inventory." << endl;
		}
	}

	return 0;
}
