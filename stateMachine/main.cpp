#include "order.hpp"

int main(void)
{
	Order order;

	OrderData* data = new OrderData();
	data->PID = 100; //
	order.startOrder(data);

	//order.Halt();

	return 0;
}

