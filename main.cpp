#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <shared_mutex>
#include <sstream>
#include <thread>
#include <chrono>

using namespace std;

class Message
{
public:
	Message(const string& sender, const string& receiver, const string& text)
		: _sender(sender), _receiver(receiver), _text(text) {}

	friend ostream& operator <<(ostream& os, const Message& message_obj)
	{
		os << message_obj._sender;
		os << '\t';
		os << message_obj._receiver;
		os << '\t';
		os << message_obj._text;
		return os;
	}

private:
	string _sender;
	string _receiver;
	string _text;
};

class Logger
{
public:
	Logger(const string& filename);
	~Logger();
	void writeLog(const Message& msg);
	string readLog(int rowCount);

	void writer(int id, const Message& msg);
	void reader(int rowCount);

private:
	fstream _logfile;
	string _filename;
	shared_mutex _shared_mutex;
	mutex mtx;
};

Logger::Logger(const string& filename) : _filename(filename)
{
	_logfile.open(filename, ios::out | ios::in | ios::trunc);
	if (!_logfile.is_open())
		cout << "Could not open file Log.txt\n";
}

Logger::~Logger()
{
	_logfile.close();
}

void Logger::writeLog(const Message& msg)
{
	_shared_mutex.lock();
	_logfile.seekp(0, ios::end);
	_logfile << msg << '\n';
	_shared_mutex.unlock();
}

string Logger::readLog(int rowCount)
{
	lock_guard<shared_mutex> lock(_shared_mutex);
	_logfile.seekg(0, ios::beg);
	string row;
	int count = 1;
	while (!_logfile.eof())
	{
		getline(_logfile, row);
		if (count == rowCount)
		{
			return row;
		}
		count++;
	}
	return " ";
}

void Logger::writer(int id, const Message& msg) {
	this_thread::sleep_for(chrono::milliseconds(300));
	writeLog(msg);
	lock_guard<mutex> lock(mtx);
	cout << "Thread ID " << id << "\tWrite Log: " << msg << "\n";
}


void Logger::reader(int rowCount) {
	this_thread::sleep_for(chrono::milliseconds(300));
	string row = readLog(rowCount);
	lock_guard<mutex> lock(mtx);
	if (!row.empty()) {
		istringstream input{ row };
		string sender, receiver, text;
		getline(input, sender, '\t');
		getline(input, receiver, '\t');
		getline(input, text);
		cout << "Read Log: Row " << rowCount << "\tSender: " << sender << "\tReceiver : " << receiver << "\tText : " << text << "\n";
	}
	else
		cout << "Row " << rowCount << " not found\n";
}


int main()
{
	Logger log("log.txt");


	vector<Message> messages;
	vector<thread> writeThreads{};
	vector<thread> readThreads{};
	string receiver, text;


	for (size_t i = 1; i <= 10; i++)
	{
		receiver = "User" + to_string(i);
		text = "text, " + receiver + "!";
		messages.emplace_back("receiver", receiver, text);
	}


	for (int i = 0; i < messages.size(); i++)
	{
		thread t(&Logger::writer, &log, i + 1, ref(messages[i]));
		writeThreads.emplace_back(move(t));
	}


	for (int i = 0; i < 3; i++)
	{
		thread t(&Logger::reader, &log, i + 1);
		readThreads.emplace_back(move(t));
	}


	for (auto& t : writeThreads)
		t.join();
	for (auto& t : readThreads)
		t.join();

	return 0;
}
