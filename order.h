
#ifndef ORDER_H_
#define ORDER_H_

using namespace std;

class item_order {
public:
	string item;
	int quantity;
};

typedef struct order {

	int orderNo;
	double price;
	string customer;
	bool prime;
	item_order item[5];
}order;

#endif /* ORDER_H_ */
