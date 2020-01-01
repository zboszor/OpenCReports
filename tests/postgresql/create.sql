create user ocrpt;
create database ocrpttest owner ocrpt;
\c ocrpttest ocrpt

create table flintstones (id serial, name text, property text, age int, adult bool);
insert into flintstones (name, property, age, adult)
values
('Fred Flintstone','strong',31,true),
('Wilma Flintstone','charming',28,true),
('Pebbles Flintstone','young',0.5,false);
