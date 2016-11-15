#ifndef WizFi310RingBuffer_h
#define WizFi310RingBuffer_h


class WizFi310RingBuffer
{
public:
	WizFi310RingBuffer(unsigned int size);
	~WizFi310RingBuffer();

	void reset();
	void init();
	void push(char c);
	int getPos();
	bool endsWith(const char* str);
	void getStr(char * destination, unsigned int skipChars);
	void getStrN(char * destination, unsigned int skipChars, unsigned int num);

	char *getStr();

private:

	unsigned int _size;
	char* ringBuf;
	char* ringBufEnd;
	char* ringBufP;

};

#endif

