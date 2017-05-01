DROP TABLE inventory;
DROP TABLE orders;

CREATE TABLE inventory(
       pid int8,
       wid int4, -- lists all warehouses that have this produce. e.g. if warehouse 1,2,3 have this, this field would be "1,2,3" where warehouses are separated by comma
       q int8,
       descr text
);

CREATE INDEX items ON inventory (pid,descr);

CREATE TABLE orders(
       pid int8,
       wid int4,
       q int8,
       descr text,
       status int4, -- 0 for first entering the database, 1 for when backend has received the order, 2 for when the product is shipping
       track_no int8, -- tracking ID, which will be updated from information received from UPS. when the buy request is first created, have this field be -1.
       adx int4,
       ady int4,
       userID int4,
       category text
);

CREATE INDEX orderID ON orders (pid,wid,status);
