#include <iostream>
#include "CircBuf.h"
#include "test.h"

using namespace std;

int main()
{
	cout << "\n=== Testing CircBuf with Default Constructor ===" << endl;
	CircBuf ab;
	test_named("Default constructor creates buffer with capacity 0", ab.capacity() == 0);
	
	ab.insert("ONE", 3);
	test_named("Insert 3 characters gives size 3", ab.size() == 3);
	test_named("Get 2 characters returns 'ON'", ab.get(2) == "ON");
	test_named("After getting 2 chars, size is 1", ab.size() == 1);
	test_named("Examine shows remaining 'E' at position 2", ab.examine() == "[--E-----]");
	
	ab.insert("TWO");
	test_named("Insert string 'TWO' wraps around correctly", ab.examine() == "[--ETWO--]");
	
	ab.get();
	ab.insert("THREE");
	test_named("After get and insert 'THREE', data wraps properly", ab.examine() == "[REETWOTH]");
	test_named("Get 3 characters returns 'TWO'", ab.get(3) == "TWO");
	test_named("After getting 'TWO', buffer state is correct", ab.examine() == "[REE---TH]");
	test_named("Size is 5 after operations", ab.size() == 5);
	test_named("Capacity remains 8", ab.capacity() == 8);
	
	ab.insert("FOUR", 4);
	test_named("Insert 4 more chars causes buffer growth, size is 9", ab.size() == 9);
	test_named("Buffer grew to capacity 16", ab.capacity() == 16);
	test_named("Data recentered correctly after growth", ab.examine() == "[THREEFOUR-------]");
	
	string threefour = ab.get(10);
	test_named("Get 10 chars (more than available) returns all 9", threefour == "THREEFOUR");
	test_named("Buffer is empty after getting all data", ab.size() == 0);
	
	cout << "\n=== Testing CircBuf with Constructor Parameter ===" << endl;
	CircBuf cb(1);
	test_named("Constructor with 1 rounds up to CHUNK size 8", cb.capacity() == 8);
	
	cb.insert("abcdefgh", 8);
	test_named("Insert 8 chars fills buffer exactly", cb.examine() == "[abcdefgh]");
	test_named("Size equals capacity after filling", cb.size() == 8);
	test_named("Capacity is 8", cb.capacity() == 8);
	test_named("Get single char returns 'a'", cb.get() == 'a');
	test_named("After getting 'a', examine shows gap at start", cb.examine() == "[-bcdefgh]");
	test_named("Size is 7 after getting one char", cb.size() == 7);
	
	cb.insert("i");
	test_named("Insert 'i' fills the gap, wraparound works", cb.examine() == "[ibcdefgh]");
	test_named("Size is 8 after wraparound insert", cb.size() == 8);
	
	string s = cb.get(3);
	test_named("Get 3 chars returns 'bcd'", s == "bcd");
	test_named("Get 0 characters returns empty string", cb.get(0) == "");
	test_named("Size is 5 after getting 3 chars", cb.size() == 5);
	test_named("Buffer state shows gaps after partial get", cb.examine() == "[i---efgh]");
	
	cb.insert("ABCDEFGH", 6);
	test_named("Insert 6 chars (not all 8) causes growth, size is 11", cb.size() == 11);
	test_named("Data recentered after growth due to wraparound", cb.examine() == "[efghiABCDEF-----]");
	
	s = cb.get(6);
	test_named("Get 6 chars returns 'efghiA'", s == "efghiA");
	test_named("Size is 5 after getting 6", cb.size() == 5);
	test_named("Buffer shows remaining data after large get", cb.examine() == "[------BCDEF-----]");
	
	cb.insert("1234567890");
	test_named("Insert 10-char string causes wraparound, size is 15", cb.size() == 15);
	test_named("Wraparound insert displays correctly", cb.examine() == "[67890-BCDEF12345]");
	
	cb.insert("xyz");
	test_named("Insert 'xyz' causes growth and recentering", cb.examine() == "[BCDEF1234567890xyz------]");
	test_named("Get 17 chars (more than size 18) returns 17", cb.get(17) == "BCDEF1234567890xy");
	test_named("Buffer shows single char 'z' remaining", cb.examine() == "[-----------------z------]");
	
	cout << "\n=== Testing Buffer Shrinking ===" << endl;
	cb.shrink();
	test_named("Shrink reduces buffer to minimal size", cb.examine() == "[z-------]");
	test_named("Size is 1 after shrink", cb.size() == 1);
	test_named("Capacity is 8 after shrink", cb.capacity() == 8);
	test_named("Get last char 'z'", cb.get() == 'z');
	
	cb.shrink();
	test_named("Shrink empty buffer gives capacity 0", cb.examine() == "[]");
	test_named("Size is 0 after shrinking empty buffer", cb.size() == 0);
	test_named("Capacity is 0 after shrinking empty buffer", cb.capacity() == 0);
	
	cout << "\n=== Testing Flush Operation ===" << endl;
	cb.insert("The Quick Brown Fox Jumped Over the Lazy Poodle");
	test_named("Insert long string, size is 47", cb.size() == 47);
	test_named("Flush returns all data and empties buffer", 
	           cb.flush() == "The Quick Brown Fox Jumped Over the Lazy Poodle");
	test_named("Size is 0 after flush", cb.size() == 0);
	
	cout << "\n=== Testing Large Initial Capacity ===" << endl;
	CircBuf xb(511);
	test_named("Constructor with 511 gives size 0", xb.size() == 0);
	test_named("Constructor with 511 rounds up to 512", xb.capacity() == 512);
	
	report_();
	reset_();
}
