DROP TABLE inventory;
DROP TABLE orders;

CREATE TABLE inventory(
       pid int8,
       wid int8,
       q int8,
       descr text
);

CREATE INDEX items ON inventory (pid,descr);

CREATE TABLE orders(
       pid int8,
       wid int8,
       q int8,
       descr text,
       status int4,
       track_no int8,
       adx int4,
       ady int4
);

CREATE INDEX orderID ON orders (pid,wid,status);
