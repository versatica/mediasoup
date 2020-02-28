#include "common.hpp"
#include "Utils.hpp"
#include <catch.hpp>

using namespace Utils;

SCENARIO("String::ToLowerCase()")
{
	std::string str;

	str = "Foo";
	String::ToLowerCase(str);
	REQUIRE(str == "foo");

	str = "Foo!œ";
	String::ToLowerCase(str);
	REQUIRE(str == "foo!œ");
}

SCENARIO("String::Base64Encode()")
{
	std::string data;
	std::string encoded;
	size_t outLen;
	uint8_t* decodedPtr;
	std::string decoded;

	data       = "abcd";
	encoded    = String::Base64Encode(data);
	decodedPtr = String::Base64Decode(encoded, outLen);
	decoded    = std::string(reinterpret_cast<char*>(decodedPtr), outLen);
	REQUIRE(encoded == "YWJjZA==");
	REQUIRE(decoded == data);

	data       = "Iñaki";
	encoded    = String::Base64Encode(data);
	decodedPtr = String::Base64Decode(encoded, outLen);
	decoded    = std::string(reinterpret_cast<char*>(decodedPtr), outLen);
	REQUIRE(encoded == "ScOxYWtp");
	REQUIRE(decoded == data);

	data =
	  "kjsh 23 å∫∂ is89 ∫¶ §∂¶ i823y kjahsd 234u asd kasjhdii7682342 asdkjhaskjsahd   k jashd kajsdhaksjdh skadhkjhkjh       askdjhasdkjahs uyqiwey aså∫∂¢∞¬∫∂ ashksajdh kjasdhkajshda s kjahsdkjas 987897as897 97898623 9s kjsgå∫∂ 432å∫ƒ∂ å∫#¢ ouyqwiuyais kajsdhiuye  ajshkkSAH SDFYÑÑÑ å∫∂Ω 87253847b asdbuiasdi as kasuœæ€\n321";
	encoded    = String::Base64Encode(data);
	decodedPtr = String::Base64Decode(encoded, outLen);
	decoded    = std::string(reinterpret_cast<char*>(decodedPtr), outLen);
	REQUIRE(
	  encoded ==
	  "a2pzaCAyMyDDpeKIq+KIgiBpczg5IOKIq8K2IMKn4oiCwrYgaTgyM3kga2phaHNkIDIzNHUgYXNkIGthc2poZGlpNzY4MjM0MiBhc2Rramhhc2tqc2FoZCAgIGsgamFzaGQga2Fqc2RoYWtzamRoIHNrYWRoa2poa2poICAgICAgIGFza2RqaGFzZGtqYWhzIHV5cWl3ZXkgYXPDpeKIq+KIgsKi4oiewqziiKviiIIgYXNoa3NhamRoIGtqYXNkaGthanNoZGEgcyBramFoc2RramFzIDk4Nzg5N2FzODk3IDk3ODk4NjIzIDlzIGtqc2fDpeKIq+KIgiA0MzLDpeKIq8aS4oiCIMOl4oirI8KiIG91eXF3aXV5YWlzIGthanNkaGl1eWUgIGFqc2hra1NBSCBTREZZw5HDkcORIMOl4oir4oiCzqkgODcyNTM4NDdiIGFzZGJ1aWFzZGkgYXMga2FzdcWTw6bigqwKMzIx");
	REQUIRE(decoded == data);
}
