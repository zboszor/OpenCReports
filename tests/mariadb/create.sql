create user 'ocrpt'@'%';
create user 'ocrpt'@'localhost';
create database ocrpttest;
grant all on ocrpttest.* to 'ocrpt'@'%';
grant all on ocrpttest.* to 'ocrpt'@'localhost';
\r ocrpttest

create table flintstones (id integer auto_increment primary key, name text, property text, age int, adult bool);
insert into flintstones (name, property, age, adult)
values
('Fred Flintstone','strong',31,true),
('Wilma Flintstone','charming',28,true),
('Pebbles Flintstone','young',0.5,false);

create table rubbles (id integer auto_increment primary key, name text, property text, age int, adult bool);
insert into rubbles (id, name, property, age, adult)
values
(2,'Betty Rubble','beautiful',27,true),
(1,'Barney Rubble','small',28,true);
