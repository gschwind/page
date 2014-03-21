/*
 * test_region.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include <iostream>
#include "box.hxx"
#include "region.hxx"

using namespace page;
using namespace std;


int main(int argc, char ** argv) {

	region_t<int> x(1,1,3,3);

	region_t<int> y0(1,1,1,1);
	region_t<int> y1(2,1,1,1);
	region_t<int> y2(3,1,1,1);
	region_t<int> y3(1,2,1,1);
	region_t<int> y4(2,2,1,1);
	region_t<int> y5(3,2,1,1);
	region_t<int> y6(1,3,1,1);
	region_t<int> y7(2,3,1,1);
	region_t<int> y8(3,3,1,1);


	cout << x.to_string() << " - " << y0.to_string() << " = " << (x - y0).to_string() << endl;
	cout << x.to_string() << " - " << y1.to_string() << " = " << (x - y1).to_string() << endl;
	cout << x.to_string() << " - " << y2.to_string() << " = " << (x - y2).to_string() << endl;
	cout << x.to_string() << " - " << y3.to_string() << " = " << (x - y3).to_string() << endl;
	cout << x.to_string() << " - " << y4.to_string() << " = " << (x - y4).to_string() << endl;
	cout << x.to_string() << " - " << y5.to_string() << " = " << (x - y5).to_string() << endl;
	cout << x.to_string() << " - " << y6.to_string() << " = " << (x - y6).to_string() << endl;
	cout << x.to_string() << " - " << y7.to_string() << " = " << (x - y7).to_string() << endl;
	cout << x.to_string() << " - " << y8.to_string() << " = " << (x - y8).to_string() << endl;

	return 0;

}
