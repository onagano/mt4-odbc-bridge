#include <Windows.h>
#include <stdio.h>
#include <map>
#include <tchar.h>


class Hoge {
	int id;
	TCHAR mesg[100];
public:
	Hoge() {
		printf("Hoge default constructor is called\n");
		this->id = 0;
		_tcscpy_s(this->mesg, _T("aaa"));
	};
	Hoge(int id) {
		printf("Hoge(int) constructor is called\n");
		this->id = id;
		_tcscpy_s(this->mesg, _T("bbb"));
	};
	Hoge(TCHAR *amesg) {
		printf("Hoge(TCHAR*) constructor is called\n");
		this->id = 0;
		_tcscpy_s(this->mesg, amesg);
	};
	Hoge(Hoge &h) {
		printf("Hoge copy constructor is called\n");
		this->id = h.getId();
		_tcscpy_s(this->mesg, h.getMesg());
	};
	~Hoge() {
		printf("Hoge destructor is called\n");
	};
	inline int getId() { return id; };
	inline TCHAR* getMesg() { return mesg; };
};


int maptest(int argc, TCHAR *argv[]) {
	int rc = 0;

	printf("=== Map Test Begin\n");

	using namespace std;
	map<int, Hoge*> mymap;

	Hoge hoge1;
	mymap[1] = &hoge1;
	printf("hoge1 id is %d\n", mymap[1]->getId());
	_tprintf(_T("hoge1 id is %s\n"), mymap[1]->getMesg());

	printf("Missing entry pointer is %p\n", mymap[2]);

	TCHAR *mesg = _T("foo bar!");
	_tprintf(_T("Mesg is %s\n"), mesg);
	Hoge *hoge3 = new Hoge(mesg);
	mymap[3] = hoge3;
	_tprintf(_T("hoge3 mesg is %s\n"), mymap[3]->getMesg());
	delete hoge3;
	mymap.erase(3);

	for (map<int, Hoge*>::iterator ite = mymap.begin(); ite != mymap.end(); ite++) {
		Hoge* h = ite->second;
		TCHAR* m = h ? h->getMesg() : _T("");
		_tprintf(_T("KEY %d and VALUE %s\n"), ite->first, m);
	}

	HANDLE lock;
	lock = CreateMutex(NULL, FALSE, _T("hogeLock"));

	_tprintf(_T("Will clear map with lock\n"));
	WaitForSingleObject(lock, INFINITE);
	mymap.clear();
	ReleaseMutex(lock);
	_tprintf(_T("Cleared\n"));

	CloseHandle(lock);

	printf("=== Map Test End\n");
	return rc;
}
