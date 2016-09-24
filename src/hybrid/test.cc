#include <iostream>
#include <vector>
using namespace std;

int main(int argc, char *argv[]) {

  vector<vector<int> > myvector;

  myvector.resize(10);
  for (unsigned int i = 0; i != 10; ++i)
    for (unsigned int j = 0; j != 10; ++j)
      myvector[i].push_back(i*j + j);

  for (unsigned int i = 0; i != 10; ++i) {
    for (unsigned int j = 0; j != 10; ++j)
      cout << myvector[i][j] << " ";
    cout << endl;
  }

  cout << "First column" << endl;

  for (unsigned int i = 0; i != 10; ++i)
    cout << myvector[i]

  return 0;
}
