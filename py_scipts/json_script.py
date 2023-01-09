import json
import random
import copy

#create json:
products = ["Beer", "Wine", "Champagne", "Vodka", "Rum","Rye","Tequila"]
customers = ["Tristan","Chris","Henry","Brandon"]

order = {
    "customer": "test",
    "price": 0,
    "prime": "false",
    "item" : []
}

items = {
    "item": "test",
    "quantity": 0
}

orders = []

for i in range(10):
 
    custInd = random.randint(0,len(customers) -1)
    cust = customers[custInd]
    order.update({"customer" : cust})

    theItems = []
    numProd = random.randint(1,3)
    for i in range(numProd):
        prodInd = random.randint(0,len(products) -1)
        prod = products[prodInd]

        quantity = random.randint(1,5)

        items.update({"item": prod})
        items.update({"quantity":quantity})
        newItem = copy.deepcopy(items)
        theItems.append(newItem)
    
    order.update({"item":theItems})
    newOrd = copy.deepcopy(order)
    orders.append(newOrd)


print(orders)
dictionary = {
    "orders" : []
}
dictionary.update({"orders" : orders})


productJson = {
    "product" : "",
    "amount" : -1
}

with open("sample.json", "w") as outfile:
    json.dump(dictionary,outfile,indent=4)

allProd = []
for i in products:
    tempAmount = random.randint(100,1000)
    tempProd = i
    productJson.update({"product":tempProd})
    productJson.update({"amount" : tempAmount})
    newProd = copy.deepcopy(productJson)
    allProd.append(newProd)

dictionaryInv = {
    "products" : []
}
dictionaryInv.update({"products" : allProd})


with open("inventory.json","w") as outfile2:
    json.dump(dictionaryInv,outfile2,indent=3)