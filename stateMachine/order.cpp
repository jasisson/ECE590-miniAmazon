#include <assert.h>
#include "order.hpp"
#include <iostream>

using namespace std;

// start order external event
void Order::startOrder(OrderData* pData)
{
  BEGIN_TRANSITION_MAP                    // - Current State -
    TRANSITION_MAP_ENTRY (ST_WHPURCHASE)  // ST_Idle
    TRANSITION_MAP_ENTRY (ST_WHARRIVED)   // ST_WHPurchase
    TRANSITION_MAP_ENTRY (ST_WHARRIVED)   // ST_WHArrived       
    TRANSITION_MAP_ENTRY (ST_UPSSHIP)     // ST_UPSShip     
    TRANSITION_MAP_ENTRY (ST_UPSRESP)     // ST_UPSResp
    TRANSITION_MAP_ENTRY (ST_WHTOPACK)    // ST_WHtoPack
    TRANSITION_MAP_ENTRY (ST_WHREADY)     // ST_WHReady
    TRANSITION_MAP_ENTRY (ST_WHLOAD)      // ST_WHLoad
    TRANSITION_MAP_ENTRY (ST_WHLOADED)    // ST_WHLoaded
    TRANSITION_MAP_ENTRY (ST_UPSDELIVER)  // ST_UPSDeliver
    END_TRANSITION_MAP(NULL)
}

// state machine sits here when not running
void Order::ST_Idle(EventData* pData) 
{
  // start of the state machine, transitions immediatedly to purchase
  // should add check if item is already in stock to skip purchase?
  cout << "Order::ST_Idle" << endl;
  // transition to ST_WHPurchase via an internal event
  InternalEvent(ST_WHPURCHASE);
}

// start purchase
void Order::ST_WHPurchase(OrderData* pData)
{
  cout << "Order::ST_WHPurchase" << endl;
  // add GPB purchase call here
  // transition to ST_WHArrived via an internal event
  InternalEvent(ST_WHARRIVED);
}

// wait for order confirmation
void Order::ST_WHArrived(OrderData* pData)
{
  cout << "Order::ST_WHArrived" << endl;
  //Added GPB call for arrived here

  InternalEvent(ST_UPSSHIP);
}

// Give UPS Shipping number, tell them which warehouse
void Order::ST_UPSShip(OrderData* pData)
{
  cout << "Order::ST_UPSShip" << endl;
  //Code here

  InternalEvent(ST_UPSRESP);
}

// Wait for UPS response
void Order::ST_UPSResp(OrderData* pData)
{
  cout << "Order::ST_UPSResp" << endl;
  //Code here

  InternalEvent(ST_WHTOPACK);
}

// Tell WH to pack order
void Order::ST_WHtoPack(OrderData* pData)
{
  cout << "Order::ST_WHtoPack" << endl;
  //Code here

  InternalEvent(ST_WHREADY);
}

// Wait for UPS AND WH triggers, could arrive at different times
void Order::ST_WHReady(OrderData* pData)
{
  cout << "Order::ST_WHReady" << endl;
  //Code here

  InternalEvent(ST_WHLOAD);
}

// After both events, tell WH to load
void Order::ST_WHLoad(OrderData* pData)
{
  cout << "Order::ST_WHLoad" << endl;
  //Code here

  InternalEvent(ST_WHLOADED);
}

// Receive WH Loaded
void Order::ST_WHLoaded(OrderData* pData)
{
  cout << "Order::ST_WHLoaded" << endl;
  //Code here

  InternalEvent(ST_UPSDELIVER);
}

// Tell UPS order is loaded and to deliver
void Order::ST_UPSDeliver(OrderData* pData)
{
  cout << "Order::ST_WHLoaded" << endl;
  //Code here
  //Need to add kill code here?
}
