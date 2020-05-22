/*
CMPE 230 PROJECT 1
-> Akif Faruk NANE, 2017400033
-> Furkan NANE, 2015400234
*/


#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <algorithm>
#include <map>
#include <cctype>
#include <vector>

#define uchar unsigned char
#define uint unsigned int
#define ushort unsigned short

using namespace std;

uchar memory[64 * 1024]; //64kb memory
uchar registers[16]; //8 word registers 

int numoflines = 0; //how many line I read, linecount
int writeindex = 0; //where to write lines in the memory
bool ZF = 0, CF = 0, AF = 0, SF = 0, OF = 0;

map<string, int> labels; //labels gives the line to proceed
map<string, ushort*> WordVariables; //my variables that are allocated on the memory
map<string, uchar*> ByteVariables;

map<int, int> lines; //gives the start index of a line in the memory

int nullflag1 = 0; //my null detectors for read functions, if cant read such that value, nullflag is set to 1
int nullflag2 = 0;
int nullflag3 = 0;

ifstream fcin;

//convert string to lower version, we can say it protects the original string, creates a new one
string lower(string a)
{
	transform(a.begin(), a.end(), a.begin(), ::tolower);
	return a;
}

//print the error and exit
void PrintError(string a)
{
	cout << "Error detected: " << a << endl;
	exit(0);
}

//counts white spaces
int spacecount(string& a)
{
	int res = 0;
	for (int i = 0; i < a.size(); i++)
		if (a[i] == ' ' || a[i] == '\t' || a[i] == '\n' || a[i] == '\r' || a[i] == '\f' || a[i] == '\v')
			res++;
	return res;
}

uchar read8bitValue(string& a, int& nullflag);
ushort read16bitValue(string& a, int& nullflag);
uchar readIntermediate8bitValue(string& a, int& nullflag);
ushort readIntermediate16bitValue(string& a, int& nullflag);

//deletes the white space caring about the strings and chars. example: ' ' wont be converted to '' 
string DeleteWhiteSpace(string& line)
{
	int b = 0;
	int i2 = 0;

	bool in1 = false;
	bool in2 = false;
	for (int i = 0; i < line.size(); i++)
	{
		if (line[i] == '\'')
			in1 = !in1;
		if (line[i] == '\"')
			in2 = !in2;
		if (!in1 && !in2 && (line[i] == ' ' || line[i] == '\t' || line[i] == '\n' || line[i] == '\r' || line[i] == '\f' || line[i] == '\v'))
			b++;
	}
	in1 = false;
	in2 = false;
	string x(line.size() - b, '.');
	for (int i = 0; i < line.size(); i++)
	{
		if (line[i] == '\'')
			in1 = !in1;
		if (line[i] == '\"')
			in2 = !in2;
		if (in1 || in2 || !(line[i] == ' ' || line[i] == '\t' || line[i] == '\n' || line[i] == '\r' || line[i] == '\f' || line[i] == '\v'))
			x[i2++] = line[i];
		b++;
	}
	return x;
}

//clears the comments caring about the strings and chars. example: ";" wont be converted to "
string ClearComment(string& line)
{
	bool in1 = false;
	bool in2 = false;
	int index = -1;

	for (int i = 0; i < line.size(); i++)
	{
		if (line[i] == '\'')
			in1 = !in1;
		if (line[i] == '\"')
			in2 = !in2;
		if (!in1 && !in2 && (line[i] == ';'))
		{
			index = i;
			break;
		}
	}

	if (index != -1)
	{
		string y(index, '.');
		for (int i = 0; i < y.size(); i++)
			y[i] = line[i];
		return y;
	}
	else
		return line;
}

//splits the string by seperator caring about the strings and chars 
void split(string& x, char sep, vector<string>& parsed)
{
	bool in1 = false;
	bool in2 = false;
	int index = 0;

	for (int i = 0; i < x.size(); i++)
	{
		if (x[i] == '\'')
			in1 = !in1;
		else if (x[i] == '\"')
			in2 = !in2;
		else if (x[i] == sep && !in1 && !in2)
		{
			if (i - index > 0)
			{
				parsed.push_back(x.substr(index, i - index));
				index = i + 1;
			}
		}
	}

	if (x.size() - index > 0)
		parsed.push_back(x.substr(index, x.size() - index));
}


//deletes the space in the corners
string DeleteWhiteSpaceCorners(string& line)
{
	if (line.size() == 0)return line;

	int a = 0, b = line.size() - 1;

	while (a < line.size() && (line[a] == ' ' || line[a] == '\t' || line[a] == '\n' || line[a] == '\r' || line[a] == '\f' || line[a] == '\v'))
		a++;
	while (b >= 0 && (line[b] == ' ' || line[b] == '\t' || line[b] == '\n' || line[b] == '\r' || line[b] == '\f' || line[b] == '\v'))
		b--;

	if (b < a)return "";
	if (a >= line.size())return "";
	if (b < 0)return "";
	return line.substr(a, b - a + 1);
}

//splits the string into 3 substring for variables
void splitVariableMode(string& x, char sep, vector<string>& parsed)
{
	int b = 0;

	while ((b = x.find_first_not_of(sep, b)) != string::npos)
	{
		int e = x.find_first_of(sep, b);

		if (e == -1)
			e = x.size();

		if (e - b > 0)
		{
			if (parsed.size() == 2)
			{
				e = x.size();
				string last = x.substr(b, e - b);
				parsed.push_back(DeleteWhiteSpaceCorners(last));
				return;
			}
			else
			{
				parsed.push_back(x.substr(b, e - b));
			}
		}
		b = e;
	}
}

string DeleteInvisCharacters(string& line)
{
	string res = "";
	for (int b = 0; b < line.size(); b++)
		if (!(line[b] == '\t' || line[b] == '\n' || line[b] == '\r' || line[b] == '\f' || line[b] == '\v'))
			res += line[b];
		else if (line[b] == '\t')
			res += ' ';
	return res;
}

//Reads the asm code. reads label and creates variables in this function. labels are not written into memory. 
//the value of a variable is written into memory not the name.
bool read = false;
bool readcleanline()
{
	lines[numoflines] = writeindex;

	string line;
	if (!getline(fcin, line))
		return false;

	line = ClearComment(line);
	line = DeleteInvisCharacters(line);

	bool writetomemory = true;

	vector<string> x2;
	split(line, ' ', x2);

	if (x2.size() == 0)
		return true;
	else if (x2.size() == 2 && lower(x2[0]) == "code" && lower(x2[1]) == "segment")
	{
		read = true;
		return true;
	}
	else if (x2.size() == 2 && lower(x2[0]) == "code" && lower(x2[1]) == "ends")
	{
		read = false;
		return true;
	}
	if (!read)return true;

	string x = DeleteWhiteSpaceCorners(line);
	//for reading labels
	if (x.size() > 0 && x[x.size() - 1] == ':')
	{
		string tem = x.substr(0, x.size() - 1);
		string label = DeleteWhiteSpaceCorners(tem);

		if (spacecount(label) == 0)
		{
			labels[label] = numoflines;
			writetomemory = false;
		}
	}
	//end of labels


	//for reading variables
	vector<string> parsed;
	splitVariableMode(line, ' ', parsed);
	if (parsed.size() == 3)
	{
		if (lower(parsed[1]) == "dw")
		{
			int isnull = 0;

			ushort val;
			if (parsed[2] == "?")
				val = 0;
			else
				val = readIntermediate16bitValue(parsed[2], isnull);

			if (!isnull)
			{
				uchar arr[2];
				*((ushort*)arr) = val;
				string x2(2, '.');
				x2[0] = arr[0];
				x2[1] = arr[1];
				line = x2;
				WordVariables[parsed[0]] = (ushort*)(memory + writeindex);
				//cout << "found dw var named: " << parsed[0] << " value of " << val << endl;
			}
			else
				PrintError("dw hata");

		}
		else if (lower(parsed[1]) == "db")
		{
			int isnull;
			uchar val = readIntermediate8bitValue(parsed[2], isnull);
			if (!isnull)
			{
				line = val;
				ByteVariables[parsed[0]] = memory + writeindex;
				//cout << "found db var named: " << parsed[0] << " value of " << (uint)(val) << endl;
			}
			else
				PrintError("db hata");
		}

	}

	//end of variables

	if (writetomemory)
	{
		for (int i = 0; i < line.size(); i++)
			memory[writeindex++] = line[i];

		numoflines++;
	}
	return true;
}

//gets the n-th line
string GetLine(int n)
{
	int size = lines[n + 1] - lines[n];
	string line(size, '.');
	for (int i = 0; i < size; i++)
		line[i] = memory[lines[n] + i];
	return line;
}

//returns a clean parsed line. the first one is the instruction and the others are the parameters seperated by comma ,
vector<string> GetCleanLine(int n)
{
	vector<string> parsed;

	string x = GetLine(n);

	x = ClearComment(x);//earse after ';'

	int bas = -1, bit = x.size();
	for (int i = 0; i < x.size(); i++)
		if (x[i] != ' ' && x[i] != '\t' && x[i] != '\n' && x[i] != '\r' && x[i] != '\f' && x[i] != '\v')
		{
			bas = i;
			break;
		}

	if (bas == -1)
		return parsed;

	for (int i = bas; i < x.size(); i++)
		if (x[i] == ' ' || x[i] == '\t' || x[i] == '\n' || x[i] == '\r' || x[i] == '\f' || x[i] == '\v')
		{
			bit = i;
			break;
		}

	string instruction = x.substr(bas, bit - bas);
	string remain = x.substr(bit, x.size() - bit);

	parsed.push_back(instruction);
	split(remain, ',', parsed);

	return parsed;
}

//tries to read 16bit address, if fails nullflag is set to 1.
ushort* read16bitAddress(string& a, int& nullflag)
{
	nullflag = 0;
	string la = lower(a);
	if (la == "ax")
		return (ushort*)(&registers[0]);
	else if (la == "bx")
		return (ushort*)(&registers[2]);
	else if (la == "cx")
		return (ushort*)(&registers[4]);
	else if (la == "dx")
		return (ushort*)(&registers[6]);
	else if (la == "di")
		return (ushort*)(&registers[8]);
	else if (la == "sp")
		return (ushort*)(&registers[10]);
	else if (la == "si")
		return (ushort*)(&registers[12]);
	else if (la == "bp")
		return (ushort*)(&registers[14]);
	else if (la[0] == 'w' && la[1] == '[' && la[la.size() - 1] == ']')
	{
		string b = a.substr(2, a.size() - 3);

		int val = read16bitValue(b, nullflag);
		return (ushort*)(memory + val);
	}
	else if (la[0] == '[' && la[la.size() - 1] == ']')
	{
		string b = a.substr(1, a.size() - 2);

		int val = read16bitValue(b, nullflag);
		return (ushort*)(memory + val);
	}
	else if (WordVariables.find(a) != WordVariables.end())
		return WordVariables.find(a)->second;
	//16 bit register +
	//16 bit memory W[adres] and [adres] + 
	//16 bit variable +
	nullflag = 1;
	return 0;
}

//tries to read 8bit address, if fails nullflag is set to 1.
uchar* read8bitAddress(string& a, int& nullflag)
{
	nullflag = 0;
	string la = lower(a);
	if (la == "al")
		return &registers[0];
	else if (la == "ah")
		return &registers[1];
	else if (la == "bl")
		return &registers[2];
	else if (la == "bh")
		return &registers[3];
	else if (la == "cl")
		return &registers[4];
	else if (la == "ch")
		return &registers[5];
	else if (la == "dl")
		return &registers[6];
	else if (la == "dh")
		return &registers[7];
	else if (la[0] == 'b' && a[1] == '[' && a[a.size() - 1] == ']')
	{
		string b = a.substr(2, a.size() - 3);

		int val = read16bitValue(b, nullflag);
		return (uchar*)(memory + val);
	}
	else if (a[0] == '[' && a[a.size() - 1] == ']')
	{
		string b = a.substr(1, a.size() - 2);

		int val = read16bitValue(b, nullflag);
		return (uchar*)(memory + val);
	}
	else if (ByteVariables.find(a) != ByteVariables.end())
		return ByteVariables.find(a)->second;
	//8 bit variable + 
	//8 bit register + 
	//8 bit memory +
	//b[adres] +
	//[adres] +
	nullflag = 1;
	return 0;
}

//if it's a number.
bool isnumber(string& s)
{
	if (s.empty())
		return false;
	for (int i = 0; i < s.size(); i++)
		if (!isdigit(s[i]))return false;
	return true;
}

//reads hexa value of char, if char is not hexa then returns -1
//could make this two lines b-'0', b - 'a' + 10, but didnt prefer :D
int hexadigitvalue(char b)
{
	if (b == '0')return 0;
	else if (b == '1')return 1;
	else if (b == '2')return 2;
	else if (b == '3')return 3;
	else if (b == '4')return 4;
	else if (b == '5')return 5;
	else if (b == '6')return 6;
	else if (b == '7')return 7;
	else if (b == '8')return 8;
	else if (b == '9')return 9;
	else if (b == 'a')return 10;
	else if (b == 'b')return 11;
	else if (b == 'c')return 12;
	else if (b == 'd')return 13;
	else if (b == 'e')return 14;
	else if (b == 'f')return 15;
	else if (b == 'A')return 10;
	else if (b == 'B')return 11;
	else if (b == 'C')return 12;
	else if (b == 'D')return 13;
	else if (b == 'E')return 14;
	else if (b == 'F')return 15;
	return -1;
}

//reads hexa number
int hexanumber(string& b)
{
	int val = 0;
	int carp = 1;
	for (int i = b.size() - 1; i >= 0; i--)
	{
		val += hexadigitvalue(b[i]) * carp;
		carp *= 16;
	}
	return val;
}

//if string is a hexa number
int ishexanumber(string& b)
{
	for (int i = b.size() - 1; i >= 0; i--)
		if (hexadigitvalue(b[i]) == -1)
			return 0;
	return 1;
}

//if the string is offset command
int ifoffset(string& a)
{
	string la = lower(a);
	if (a.size() < 6)return false;
	string rem = a.substr(6);
	if (la.find("offset") == 0)
	{
		if (WordVariables.find(rem) != WordVariables.end())
			return true;
		else if (ByteVariables.find(rem) != ByteVariables.end())
			return true;
		else
			return false;
	}
	else
		return false;
}

//then read offset comman and give the adres in the memory
int readoffset(string& a, int& nullflag)
{
	//return (int)(((long)*WordVariables.find(a.substr(6))->second) - (long)memory);
	if (a.size() < 6)
	{
		nullflag = 1;
		return -1;
	}
	string la = lower(a);
	string rem = a.substr(6);
	if (la.find("offset") == 0)
	{
		if (WordVariables.find(rem) != WordVariables.end())
		{
			nullflag = 0;
			return (int)(((long)WordVariables.find(rem)->second) - (long)memory);
		}
		else if (ByteVariables.find(rem) != ByteVariables.end())
		{
			nullflag = 0;
			return (int)(((long)ByteVariables.find(rem)->second) - (long)memory);
		}
		else
		{
			nullflag = 1;
			return -1;
		}
	}
	else
	{
		nullflag = 1;
		return -1;
	}
}

//read intermediate value, no registers, no memory
ushort readIntermediate16bitValue(string& a, int& nullflag)
{
	nullflag = 0;
	string la = lower(a);

	if (isnumber(a))
	{
		int ret = stoi(a);
		nullflag = ret >= 256 * 256;
		return ret;
	}
	else if (la[a.size() - 1] == 'd')
	{
		string b = a.substr(0, a.size() - 1);
		if (isnumber(b))
		{
			int ret = stoi(b);
			nullflag = ret >= 256 * 256;
			return ret;
		}
		else
			PrintError("hata D number");
	}
	else if (la[a.size() - 1] == 'h')
	{
		string b = a.substr(0, a.size() - 1);
		if (ishexanumber(b))
			return hexanumber(b);
		else
			PrintError("hata H number");
	}
	else if (a.size() == 3 && ((a[0] == '\'' && a[2] == '\'') || (a[0] == '\"' && a[2] == '\"')))
		return a[1];
	nullflag = 1;
	return NULL;
}

//read intermediate value
uchar readIntermediate8bitValue(string& a, int& nullflag)
{
	nullflag = 0;
	string la = lower(a);
	if (isnumber(a))
	{
		int ret = stoi(a);
		nullflag = ret >= 256;
		return ret;
	}
	else if (la[a.size() - 1] == 'd')
	{
		string b = a.substr(0, a.size() - 1);
		if (isnumber(b))
		{
			int ret = stoi(b);
			nullflag = ret >= 256;
			return ret;
		}
		else
			PrintError("hata D number");
	}
	else if (la[a.size() - 1] == 'h')
	{
		string b = a.substr(0, a.size() - 1);
		if (ishexanumber(b))
		{
			int ret = hexanumber(b);
			nullflag = ret >= 256;
			return ret;
		}
		else
			PrintError("hata H number");
	}
	else if (a.size() == 3 && ((a[0] == '\'' && a[2] == '\'') || (a[0] == '\"' && a[2] == '\"')))
		return a[1];
	nullflag = 1;
	return NULL;
}

//read 16bit value
ushort read16bitValue(string& a, int& nullflag)
{
	nullflag = 0;
	string la = lower(a);
	if (la == "ax")
		return *(ushort*)(&registers[0]);
	else if (la == "bx")
		return *(ushort*)(&registers[2]);
	else if (la == "cx")
		return *(ushort*)(&registers[4]);
	else if (la == "dx")
		return *(ushort*)(&registers[6]);
	else if (la == "di")
		return *(ushort*)(&registers[8]);
	else if (la == "sp")
		return *(ushort*)(&registers[10]);
	else if (la == "si")
		return *(ushort*)(&registers[12]);
	else if (la == "bp")
		return *(ushort*)(&registers[14]);
	else if (isnumber(a))
	{
		int ret = stoi(a);
		nullflag = ret >= 256 * 256;
		return ret;
	}
	else if (WordVariables.find(a) != WordVariables.end())
		return *WordVariables.find(a)->second;
	else if (ifoffset(a))
	{
		return readoffset(a, nullflag);
	}
	else if (la[a.size() - 1] == 'd')
	{
		string b = a.substr(0, a.size() - 1);
		if (isnumber(b))
		{
			int ret = stoi(b);
			nullflag = ret >= 256 * 256;
			return ret;
		}
		else
			PrintError("hata D number");
	}
	else if (la[a.size() - 1] == 'h')
	{
		string b = a.substr(0, a.size() - 1);
		if (ishexanumber(b))
			return hexanumber(b);
		else
			PrintError("hata H number");
	}
	else if (a.size() == 3 && ((a[0] == '\'' && a[2] == '\'') || (a[0] == '\"' && a[2] == '\"')))
		return a[1];
	else if (la[0] == 'w' && a[1] == '[' && a[a.size() - 1] == ']')
	{
		string b = a.substr(2, a.size() - 3);

		int val = read16bitValue(b, nullflag);
		ushort* a = (ushort*)(memory + val);
		return *a;
	}
	else if (a[0] == '[' && a[a.size() - 1] == ']')
	{
		string b = a.substr(1, a.size() - 2);

		int val = read16bitValue(b, nullflag);
		ushort* a = (ushort*)(memory + val);
		return *a;//???
	}
	//number +
	//hexanumber? + 
	//char "!" or '!' + 
	//registers + 
	//16 bit variable name +
	//W[address] where address = 16 bit register or a number or a variable +
	nullflag = 1;
	return NULL;
}

//read 8bit value
uchar read8bitValue(string& a, int& nullflag)
{
	nullflag = 0;
	string la = lower(a);
	if (la == "al")
		return registers[0];
	else if (la == "ah")
		return registers[1];
	else if (la == "bl")
		return registers[2];
	else if (la == "bh")
		return registers[3];
	else if (la == "cl")
		return registers[4];
	else if (la == "ch")
		return registers[5];
	else if (la == "dl")
		return registers[6];
	else if (la == "dh")
		return registers[7];
	else if (isnumber(a))
	{
		int ret = stoi(a);
		nullflag = ret >= 256;
		return ret;
	}
	else if (ByteVariables.find(a) != ByteVariables.end())
		return *ByteVariables.find(a)->second;
	else if (la[a.size() - 1] == 'd')
	{
		string b = a.substr(0, a.size() - 1);
		if (isnumber(b))
		{
			int ret = stoi(b);
			nullflag = ret >= 256;
			return ret;
		}
		else
			PrintError("hata D number");
	}
	else if (la[a.size() - 1] == 'h')
	{
		string b = a.substr(0, a.size() - 1);
		if (ishexanumber(b))
		{
			int ret = hexanumber(b);
			nullflag = ret >= 256;
			return ret;
		}
		else
			PrintError("hata H number");
	}
	else if (a.size() == 3 && ((a[0] == '\'' && a[2] == '\'') || (a[0] == '\"' && a[2] == '\"')))
		return a[1];
	else if (la[0] == 'b' && a[1] == '[' && a[a.size() - 1] == ']')
	{
		string b = a.substr(2, a.size() - 3);
		int val = read16bitValue(b, nullflag);
		uchar* a = (uchar*)(memory + val);
		return *a;
	}
	else if (a[0] == '[' && a[a.size() - 1] == ']')
	{
		string b = a.substr(1, a.size() - 2);
		int val = read16bitValue(b, nullflag);
		uchar* a = (uchar*)(memory + val);
		return *a;//???
	}
	//number +
	//hexanumber? + 
	//char "!" or '!' + 
	//registers + 
	//8 bit variable name +
	//B[address] where address = 16 bit register or a number or a variable +
	nullflag = 1;
	return NULL;
}

//I dont want to explain all the processing methods. the names on them are really explaining in my opinions.
void processMov(vector<string>& x)
{
	string& a = x[1];
	string& b = x[2];

	ushort* adres1 = read16bitAddress(a, nullflag1);
	uchar* adres2 = read8bitAddress(a, nullflag2);
	if (!nullflag2)
	{
		uchar val = read8bitValue(b, nullflag2);
		if (nullflag2)
		{
			ushort val = read16bitValue(b, nullflag3);
			if (!nullflag3 && !nullflag1)
				*adres1 = val;
			else
				PrintError("mov error");
		}
		else
			*adres2 = val;
	}
	else if (!nullflag1)
	{
		ushort val = read16bitValue(b, nullflag1);
		if (nullflag1)
		{
			uchar val = read8bitValue(b, nullflag3);
			if (!nullflag2 && !nullflag3)
				*adres2 = val;
			else
				PrintError("mov error");
		}
		else
			*adres1 = val;
	}
	else
		PrintError("mov error");
}

void SetFlagsAdd(uchar a, uchar b)
{
	uchar res = a + b;
	uint ia = a;
	uint ib = b;
	uint ires = ia + ib;

	ZF = (res == 0);
	CF = (ires != res);
	SF = res >= 128;

	char a1 = a;
	char b1 = b;
	char res1 = a1 + b1;
	int a2 = a1;
	int b2 = b1;
	int res2 = a2 + b2;

	OF = res2 != res1;
	AF = ((a % 16) + (b % 16) >= 16);
}

void SetFlagsAdd(ushort a, ushort b)
{
	ushort res = a + b;
	uint ia = a;
	uint ib = b;
	uint ires = ia + ib;

	ZF = (res == 0);
	CF = (ires != res);
	SF = res >= (1 << 15);

	short a1 = a;
	short b1 = b;
	short res1 = a1 + b1;
	int a2 = a1;
	int b2 = b1;
	int res2 = a2 + b2;

	OF = res2 != res1;
	AF = ((a % 16) + (b % 16) >= 16);
}

void processAdd(vector<string>& x)
{
	string& a = x[1];
	string& b = x[2];

	ushort* adres1 = read16bitAddress(a, nullflag1);
	uchar* adres2 = read8bitAddress(a, nullflag2);
	if (!nullflag2)
	{
		uchar val = read8bitValue(b, nullflag2);
		if (nullflag2)
		{
			ushort val = read16bitValue(b, nullflag3);
			if (!nullflag3 && !nullflag1)
			{
				SetFlagsAdd(*adres1, val);
				*adres1 += val;
			}
			else
				PrintError("add error");
		}
		else
		{
			SetFlagsAdd(*adres2, val);
			*adres2 += val;
		}
	}
	else if (!nullflag1)
	{
		ushort val = read16bitValue(b, nullflag1);
		if (nullflag1)
		{
			uchar val = read8bitValue(b, nullflag3);
			if (!nullflag2 && !nullflag3)
			{
				SetFlagsAdd(*adres2, val);
				*adres2 += val;
			}
			else
				PrintError("add error");
		}
		else
		{
			SetFlagsAdd(*adres1, val);
			*adres1 += val;
		}
	}
	else
		PrintError("add error");
}

void SetFlagsSub(uchar a, uchar b)
{
	uchar res = a - b;
	uint ia = a;
	uint ib = b;
	uint ires = ia - ib;

	ZF = (res == 0);
	CF = (ires != res);
	SF = res >= 128;

	char a1 = a;
	char b1 = b;
	char res1 = a1 - b1;
	int a2 = a1;
	int b2 = b1;
	int res2 = a2 - b2;

	OF = res2 != res1;

	int ra = a % 16;
	int rb = b % 16;
	AF = (ra - rb < -15);
}

void SetFlagsSub(ushort a, ushort b)
{
	ushort res = a - b;
	uint ia = a;
	uint ib = b;
	uint ires = ia - ib;

	ZF = (res == 0);
	CF = (ires != res);
	SF = res >= (1 << 15);

	short a1 = a;
	short b1 = b;
	short res1 = a1 - b1;
	int a2 = a1;
	int b2 = b1;
	int res2 = a2 - b2;

	OF = res2 != res1;
	int ra = a % 16;
	int rb = b % 16;
	AF = (ra - rb < -15);
}

void processSub(vector<string>& x)
{
	string& a = x[1];
	string& b = x[2];

	ushort* adres1 = read16bitAddress(a, nullflag1);
	uchar* adres2 = read8bitAddress(a, nullflag2);
	if (!nullflag2)
	{
		uchar val = read8bitValue(b, nullflag2);
		if (nullflag2)
		{
			ushort val = read16bitValue(b, nullflag3);
			if (!nullflag3 && !nullflag1)
			{
				SetFlagsSub(*adres1, val);
				*adres1 -= val;
			}
			else
				PrintError("sub error");
		}
		else
		{
			SetFlagsSub(*adres2, val);
			*adres2 -= val;
		}
	}
	else if (!nullflag1)
	{
		ushort val = read16bitValue(b, nullflag1);
		if (nullflag1)
		{
			uchar val = read8bitValue(b, nullflag3);
			if (!nullflag2 && !nullflag3)
			{
				SetFlagsSub(*adres2, val);
				*adres2 -= val;
			}
			else
				PrintError("sub error");
		}
		else
		{
			SetFlagsSub(*adres1, val);
			*adres1 -= val;
		}
	}
	else
		PrintError("sub error");
}

void processCmp(vector<string>& x)
{
	string& a = x[1];
	string& b = x[2];

	ushort* adres1 = read16bitAddress(a, nullflag1);
	uchar* adres2 = read8bitAddress(a, nullflag2);
	if (!nullflag2)
	{
		uchar val = read8bitValue(b, nullflag2);
		if (nullflag2)
		{
			ushort val = read16bitValue(b, nullflag3);
			if (!nullflag3 && !nullflag1)
			{
				SetFlagsSub(*adres1, val);
			}
			else
				PrintError("cmp error");
		}
		else
		{
			SetFlagsSub(*adres2, val);
		}
	}
	else if (!nullflag1)
	{
		ushort val = read16bitValue(b, nullflag1);
		if (nullflag1)
		{
			uchar val = read8bitValue(b, nullflag3);
			if (!nullflag2 && !nullflag3)
			{
				SetFlagsSub(*adres2, val);
			}
			else
				PrintError("cmp error");
		}
		else
		{
			SetFlagsSub(*adres1, val);
		}
	}
	else
		PrintError("cmp error");
}

void SetFlagsAndProcessMul(uchar a, uchar b)
{
	//ax = *((ushort*)registers)
	//dx = *((ushort*)(registers + 6))
	uchar res = a * b;

	ushort ai = a;
	ushort bi = b;
	ushort resi = ai * bi;
	if (resi > res)
		OF = CF = 1;
	else
		OF = CF = 0;

	*((ushort*)registers) = resi;
}

void SetFlagsAndProcessMul(ushort a, ushort b)
{
	//ax = *((ushort*)registers)
	//dx = *((ushort*)(registers + 6))
	ushort res = a * b;

	uint ai = a;
	uint bi = b;
	uint resi = ai * bi;
	if (resi > res)
		OF = CF = 1;
	else
		OF = CF = 0;

	*((ushort*)registers) = (ushort)(resi % (256 * 256));
	*((ushort*)(registers + 6)) = (ushort)(resi / (256 * 256));
}

void processMul(vector<string>& x)
{
	string& a = x[1];

	ushort val16 = read16bitValue(a, nullflag1);
	uchar val8 = read8bitValue(a, nullflag2);
	if (!nullflag2)
	{
		SetFlagsAndProcessMul(registers[0], val8);
	}
	else if (!nullflag1)
	{
		SetFlagsAndProcessMul(*((ushort*)registers), val16);
	}
	else
		PrintError("mul error");
}

void SetFlagsAndProcessDiv(uchar b)
{
	if (b == 0)
	{
		PrintError("Zero division error!");
	}
	//ax = *((ushort*)registers)
	//al = *(registers)
	//ah = *(registers+1)
	//dx = *((ushort*)(registers + 6))
	ushort ax = *((ushort*)registers);
	uchar remainder = ax % b;
	ushort qu = ax / ((ushort)b);

	if (qu > 0xFF)
	{
		cout << "Quotient: " << qu << endl;
		PrintError("8bit Div Overflow!");
	}


	*(registers) = qu; //al
	*(registers + 1) = remainder; //ah
}

void SetFlagsAndProcessDiv(ushort b)
{
	if (b == 0)
	{
		PrintError("Zero division error!");
	}
	//ax = *((ushort*)registers)
	//dx = *((ushort*)(registers + 6))
	ushort ax = *((ushort*)registers);
	ushort dx = *((ushort*)(registers + 6));
	uint a = dx;
	a *= 256 * 256;
	a += ax;

	ushort remainder = a % b;
	uint qu = a / ((uint)b);

	if (qu > 0xFFFF)
	{
		cout << "Quotient: " << qu << endl;
		PrintError("16bit Div Overflow!");
	}

	*((ushort*)registers) = qu;
	*((ushort*)(registers + 6)) = remainder;
}

void processDiv(vector<string>& x)
{
	string& a = x[1];

	ushort val16 = read16bitValue(a, nullflag1);
	uchar val8 = read8bitValue(a, nullflag2);
	if (!nullflag2)
	{
		SetFlagsAndProcessDiv(val8);
	}
	else if (!nullflag1)
	{
		SetFlagsAndProcessDiv(val16);
	}
	else
		PrintError("div error");
}

void processInt21h(vector<string>& x)
{
	char method = registers[1];//ah

	if (method == 1)
	{
		char x;
		cin >> x;
		registers[0] = x;
		
	}
	else if (method == 2)
	{
		cout << (registers[0] = registers[6]);
	}

}

int processJmp(vector<string>& x)
{
	if (labels.find(x[1]) != labels.end()) return labels[x[1]]; else { PrintError("the label given is not valid!"); return -10; }
}

int processJa(vector<string>& x, int currentline)
{
	if (CF == 0 && ZF == 0)
		if (labels.find(x[1]) != labels.end()) return labels[x[1]]; else { PrintError("the label given is not valid!"); return -10; }
	else
		return currentline + 1;
}

int processJae(vector<string>& x, int currentline)
{
	if (CF == 0)
		if (labels.find(x[1]) != labels.end()) return labels[x[1]]; else { PrintError("the label given is not valid!"); return -10; }
	else
		return currentline + 1;
}

int processJb(vector<string>& x, int currentline)
{
	if (CF == 1)
		if (labels.find(x[1]) != labels.end()) return labels[x[1]]; else { PrintError("the label given is not valid!"); return -10; }
	else
		return currentline + 1;
}

int processJbe(vector<string>& x, int currentline)
{
	if (CF == 1 || ZF == 1)
		if (labels.find(x[1]) != labels.end()) return labels[x[1]]; else { PrintError("the label given is not valid!"); return -10; }
	else
		return currentline + 1;
}

int processJc(vector<string>& x, int currentline)
{
	if (CF == 1)
		if (labels.find(x[1]) != labels.end()) return labels[x[1]]; else { PrintError("the label given is not valid!"); return -10; }
	else
		return currentline + 1;
}

int processJnc(vector<string>& x, int currentline)
{
	if (CF == 0)
		if (labels.find(x[1]) != labels.end()) return labels[x[1]]; else { PrintError("the label given is not valid!"); return -10; }
	else
		return currentline + 1;
}

int processJz(vector<string>& x, int currentline)
{
	if (ZF == 1)
		if (labels.find(x[1]) != labels.end()) return labels[x[1]]; else { PrintError("the label given is not valid!"); return -10; }
	else
		return currentline + 1;
}

int processJnz(vector<string>& x, int currentline)
{
	if (ZF == 0)
		if (labels.find(x[1]) != labels.end()) return labels[x[1]]; else { PrintError("the label given is not valid!"); return -10; }
	else
		return currentline + 1;
}

int processJe(vector<string>& x, int currentline)
{
	if (ZF == 1)
		if (labels.find(x[1]) != labels.end()) return labels[x[1]]; else { PrintError("the label given is not valid!"); return -10; }
	else
		return currentline + 1;
}

int processJne(vector<string>& x, int currentline)
{
	if (ZF == 0)
		if (labels.find(x[1]) != labels.end()) return labels[x[1]]; else { PrintError("the label given is not valid!"); return -10; }
	else
		return currentline + 1;
}

int processJnae(vector<string>& x, int currentline)
{
	if (CF == 1)
		if (labels.find(x[1]) != labels.end()) return labels[x[1]]; else { PrintError("the label given is not valid!"); return -10; }
	else
		return currentline + 1;
}

int processJnbe(vector<string>& x, int currentline)
{
	if (CF == 0 && ZF == 0)
		if (labels.find(x[1]) != labels.end()) return labels[x[1]]; else { PrintError("the label given is not valid!"); return -10; }
	else
		return currentline + 1;
}

int processJna(vector<string>& x, int currentline)
{
	if (CF == 1 || ZF == 1)
		if (labels.find(x[1]) != labels.end()) return labels[x[1]]; else { PrintError("the label given is not valid!"); return -10; }
	else
		return currentline + 1;
}

int processJnb(vector<string>& x, int currentline)
{
	if (CF == 0)
		if (labels.find(x[1]) != labels.end()) return labels[x[1]]; else { PrintError("the label given is not valid!"); return -10; }
	else
		return currentline + 1;
}

void processDec(vector<string>& x)
{
	string& a = x[1];

	ushort* adres1 = read16bitAddress(a, nullflag1);
	uchar* adres2 = read8bitAddress(a, nullflag2);
	if (!nullflag2)
	{
		SetFlagsSub((*adres2), 1);
		(*adres2) -= 1;
	}
	else if (!nullflag1)
	{
		SetFlagsSub((*adres1), 1);
		(*adres1) -= 1;
	}
	else
		PrintError("hata Dec");
}

void processInc(vector<string>& x)
{
	string& a = x[1];

	ushort* adres1 = read16bitAddress(a, nullflag1);
	uchar* adres2 = read8bitAddress(a, nullflag2);
	if (!nullflag2)
	{
		SetFlagsAdd((*adres2), 1);
		(*adres2) += 1;
	}
	else if (!nullflag1)
	{
		SetFlagsAdd((*adres1), 1);
		(*adres1) += 1;
	}
	else
		PrintError("hata Dec");
}

void processPush(vector<string>& x)
{
	int nullflag;
	ushort val = read16bitValue(x[1], nullflag);

	if (!nullflag)
	{
		ushort* sp = (ushort*)(registers + 10);
		ushort* address = (ushort*)(memory + *sp);
		*address = val;

		*sp -= 2;
	}
	else
	{
		PrintError("hata push");
	}
}

void processPop(vector<string>& x)
{
	int nullflag;
	ushort* var = read16bitAddress(x[1], nullflag);

	if (!nullflag)
	{
		ushort* sp = (ushort*)(registers + 10);
		if(*sp == 0xFFFE)
		{
			PrintError("popta eleman yok");
		}
		*sp += 2;
		ushort* address = (ushort*)(memory + *sp);
		*var = *address;
	}
	else
	{
		PrintError("hata pop");
	}

}

void SetFlagsAndProcessAnd(uchar& a, uchar b)
{
	a = a & b;
	OF = CF = 0;
	ZF = a == 0;
	SF = a >= 128;
}

void SetFlagsAndProcessAnd(ushort& a, ushort b)
{
	a = a & b;
	OF = CF = 0;
	ZF = a == 0;
	SF = a >= (1 << 15);
}

void SetFlagsAndProcessOr(uchar& a, uchar b)
{
	a = a | b;
	OF = CF = 0;
	ZF = a == 0;
	SF = a >= 128;
}

void SetFlagsAndProcessOr(ushort& a, ushort b)
{
	a = a | b;
	OF = CF = 0;
	ZF = a == 0;
	SF = a >= (1 << 15);
}

void SetFlagsAndProcessXor(uchar& a, uchar b)
{
	a = a ^ b;
	OF = CF = 0;
	ZF = a == 0;
	SF = a >= 128;
}

void SetFlagsAndProcessXor(ushort& a, ushort b)
{
	a = a ^ b;
	OF = CF = 0;
	ZF = a == 0;
	SF = a >= (1 << 15);
}

void SetFlagsAndProcessNot(uchar& a)
{
	a = ~a;
	OF = CF = 0;
	ZF = a == 0;
	SF = a >= 128;
}

void SetFlagsAndProcessNot(ushort& a)
{
	a = ~a;
	OF = CF = 0;
	ZF = a == 0;
	SF = a >= (1 << 15);
}

void processBitOperations(vector<string>& x)
{
	string& a = x[1];
	string& b = x[2];

	ushort* adres1 = read16bitAddress(a, nullflag1);
	uchar* adres2 = read8bitAddress(a, nullflag2);
	if (!nullflag2)
	{
		uchar val = read8bitValue(b, nullflag2);
		if (nullflag2)
		{
			ushort val = read16bitValue(b, nullflag3);
			if (!nullflag3 && !nullflag1)
			{
				if (x[0] == "and")
					SetFlagsAndProcessAnd(*adres1, val);
				else if (x[0] == "or")
					SetFlagsAndProcessOr(*adres1, val);
				else if (x[0] == "xor")
					SetFlagsAndProcessXor(*adres1, val);
			}
			else
				PrintError("bitwise error");
		}
		else
		{
			if (x[0] == "and")
				SetFlagsAndProcessAnd(*adres2, val);
			else if (x[0] == "or")
				SetFlagsAndProcessOr(*adres2, val);
			else if (x[0] == "xor")
				SetFlagsAndProcessXor(*adres2, val);
		}
	}
	else if (!nullflag1)
	{
		ushort val = read16bitValue(b, nullflag1);
		if (nullflag1)
		{
			uchar val = read8bitValue(b, nullflag3);
			if (!nullflag2 && !nullflag3)
			{
				if (x[0] == "and")
					SetFlagsAndProcessAnd(*adres2, val);
				else if (x[0] == "or")
					SetFlagsAndProcessOr(*adres2, val);
				else if (x[0] == "xor")
					SetFlagsAndProcessXor(*adres2, val);
			}
			else
				PrintError("bitwise error");
		}
		else
		{
			if (x[0] == "and")
				SetFlagsAndProcessAnd(*adres1, val);
			else if (x[0] == "or")
				SetFlagsAndProcessOr(*adres1, val);
			else if (x[0] == "xor")
				SetFlagsAndProcessXor(*adres1, val);
		}
	}
	else
		PrintError("bitwise error");
}

void processNot(vector<string>& x)
{
	string& a = x[1];
	ushort* adres1 = read16bitAddress(a, nullflag1);
	uchar* adres2 = read8bitAddress(a, nullflag2);
	if (!nullflag2)
		SetFlagsAndProcessNot(*adres2);
	else if (!nullflag1)
		SetFlagsAndProcessNot(*adres1);
	else
		PrintError("bitwise not error");
}

void SetFlagsAndProcessRcl(ushort& a, int b)
{
	for (int i = 0; i < b; i++)
	{
		SF = a >= (1 << 15);
		int newCF = SF;
		a = a << 1;
		a += CF;
		CF = newCF;
	}
	SF = a >= (1 << 15);
	ZF = a == 0;
}

void SetFlagsAndProcessRcl(uchar& a, int b)
{
	for (int i = 0; i < b; i++)
	{
		SF = a >= (1 << 7);
		int newCF = SF;
		a = a << 1;
		a += CF;
		CF = newCF;
	}
	SF = a >= (1 << 7);
	ZF = a == 0;
}

void SetFlagsAndProcessRcr(ushort& a, int b)
{
	for (int i = 0; i < b; i++)
	{
		int newCF = a % 2;
		a = a >> 1;
		a += CF * (1 << 15);
		CF = newCF;
	}
	SF = a >= (1 << 15);
	ZF = a == 0;
}

void SetFlagsAndProcessRcr(uchar& a, int b)
{
	for (int i = 0; i < b; i++)
	{
		int newCF = a % 2;
		a = a >> 1;
		a += CF * (1 << 7);
		CF = newCF;
	}
	SF = a >= (1 << 7);
	ZF = a == 0;
}

void processRcl(vector<string>& x)
{
	string& a = x[1];
	string& b = x[2];

	ushort* adres1 = read16bitAddress(a, nullflag1);
	uchar* adres2 = read8bitAddress(a, nullflag2);
	int val = readIntermediate8bitValue(b, nullflag3);
	string lb = lower(b);

	if (lb == "cl")
	{
		int val = registers[4];
		if (!nullflag2)
			SetFlagsAndProcessRcl(*adres2, val);
		else if (!nullflag1)
			SetFlagsAndProcessRcl(*adres1, val);
		else
			PrintError("Rcl error");
	}
	else if (!nullflag3)
	{
		if (val >= 32)
			PrintError("Rcl error");
		else
		{
			if (!nullflag2)
				SetFlagsAndProcessRcl(*adres2, val);
			else if (!nullflag1)
				SetFlagsAndProcessRcl(*adres1, val);
			else
				PrintError("Rcl error");
		}
	}
	else
		PrintError("Rcl error");
}

void processRcr(vector<string>& x)
{
	string& a = x[1];
	string& b = x[2];

	ushort* adres1 = read16bitAddress(a, nullflag1);
	uchar* adres2 = read8bitAddress(a, nullflag2);
	int val = readIntermediate8bitValue(b, nullflag3);
	string lb = lower(b);

	if (lb == "cl")
	{
		int val = registers[4];
		if (!nullflag2)
			SetFlagsAndProcessRcr(*adres2, val);
		else if (!nullflag1)
			SetFlagsAndProcessRcr(*adres1, val);
		else
			PrintError("Rcr error");
	}
	else if (!nullflag3)
	{
		if (val >= 32)
			PrintError("Rcr error");
		else
		{
			if (!nullflag2)
				SetFlagsAndProcessRcr(*adres2, val);
			else if (!nullflag1)
				SetFlagsAndProcessRcr(*adres1, val);
			else
				PrintError("Rcr error");
		}
	}
	else
		PrintError("Rcr error");
}

void SetFlagsAndProcessShl(ushort& a, int b)
{
	uint a2 = a;
	a2 = a2 << b;

	a = a << b;

	CF = a != a2;


	SF = a >= (1 << 15);
	ZF = a == 0;
}

void SetFlagsAndProcessShl(uchar& a, int b)
{
	uint a2 = a;
	a2 = a2 << b;

	a = a << b;

	CF = a != a2;
	SF = a >= (1 << 7);
	ZF = a == 0;
}

void SetFlagsAndProcessShr(ushort& a, int b)
{
	a = a >> b;
	CF = SF = 0;
	ZF = a == 0;
}

void SetFlagsAndProcessShr(uchar& a, int b)
{
	a = a >> b;
	CF = SF = 0;
	ZF = a == 0;
}

void processShl(vector<string>& x)
{
	string& a = x[1];
	string& b = x[2];

	ushort* adres1 = read16bitAddress(a, nullflag1);
	uchar* adres2 = read8bitAddress(a, nullflag2);
	int val = readIntermediate8bitValue(b, nullflag3);
	string lb = lower(b);

	if (lb == "cl")
	{
		int val = registers[4];
		if (!nullflag2)
			SetFlagsAndProcessShl(*adres2, val);
		else if (!nullflag1)
			SetFlagsAndProcessShl(*adres1, val);
		else
			PrintError("Shl error");
	}
	else if (!nullflag3)
	{
		if (val >= 32)
			PrintError("Shl error");
		else
		{
			if (!nullflag2)
				SetFlagsAndProcessShl(*adres2, val);
			else if (!nullflag1)
				SetFlagsAndProcessShl(*adres1, val);
			else
				PrintError("Shl error");
		}
	}
	else
		PrintError("Shl error");
}

void processShr(vector<string>& x)
{
	string& a = x[1];
	string& b = x[2];

	ushort* adres1 = read16bitAddress(a, nullflag1);
	uchar* adres2 = read8bitAddress(a, nullflag2);
	int val = readIntermediate8bitValue(b, nullflag3);
	string lb = lower(b);

	if (lb == "cl")
	{
		int val = registers[4];
		if (!nullflag2)
			SetFlagsAndProcessShr(*adres2, val);
		else if (!nullflag1)
			SetFlagsAndProcessShr(*adres1, val);
		else
			PrintError("Shr error");
	}
	else if (!nullflag3)
	{
		if (val >= 32)
			PrintError("Shr error");
		else
		{
			if (!nullflag2)
				SetFlagsAndProcessShr(*adres2, val);
			else if (!nullflag1)
				SetFlagsAndProcessShr(*adres1, val);
			else
				PrintError("Shr error");
		}
	}
	else
		PrintError("Shr error");
}

void process(int n)
{
	if (n == numoflines)
		PrintError("no int 20h");

	vector<string> x = GetCleanLine(n);
	for (int i = 0; i < x.size(); i++)
		x[i] = DeleteWhiteSpace(x[i]);
	x[0] = lower(x[0]);

	if (x.size() == 0)
	{

	}
	else if (x.size() == 3 && x[0] == "mov")
	{
		processMov(x);
	}
	else if (x.size() == 3 && x[0] == "add")
	{
		processAdd(x);
	}
	else if (x.size() == 3 && x[0] == "sub")
	{
		processSub(x);
	}
	else if (x.size() == 2 && x[0] == "mul")
	{
		processMul(x);
	}
	else if (x.size() == 2 && x[0] == "div")
	{
		processDiv(x);
	}
	else if (x.size() == 2 && x[0] == "dec")
	{
		processDec(x);
	}
	else if (x.size() == 2 && x[0] == "inc")
	{
		processInc(x);
	}
	else if (x.size() == 2 && x[0] == "pop")
	{
		processPop(x);
	}
	else if (x.size() == 2 && x[0] == "push")
	{
		processPush(x);
	}
	else if (x.size() == 3 && (x[0] == "and" || x[0] == "or" || x[0] == "xor"))
	{
		processBitOperations(x);
	}
	else if (x.size() == 2 && x[0] == "not")
	{
		processNot(x);
	}
	else if (x.size() == 3 && x[0] == "cmp")
	{
		processCmp(x);
	}
	else if (x.size() == 2 && x[0] == "jmp")
	{
		process(processJmp(x));
		return;
	}
	else if (x.size() == 2 && x[0] == "jz")
	{
		process(processJz(x, n));
		return;
	}
	else if (x.size() == 2 && x[0] == "jnz")
	{
		process(processJnz(x, n));
		return;
	}
	else if (x.size() == 2 && x[0] == "je")
	{
		process(processJe(x, n));
		return;
	}
	else if (x.size() == 2 && x[0] == "jne")
	{
		process(processJne(x, n));
		return;
	}
	else if (x.size() == 2 && x[0] == "ja")
	{
		process(processJa(x, n));
		return;
	}
	else if (x.size() == 2 && x[0] == "jae")
	{
		process(processJae(x, n));
		return;
	}
	else if (x.size() == 2 && x[0] == "jb")
	{
		process(processJb(x, n));
		return;
	}
	else if (x.size() == 2 && x[0] == "jbe")
	{
		process(processJbe(x, n));
		return;
	}
	else if (x.size() == 2 && x[0] == "jnae")
	{
		process(processJnae(x, n));
		return;
	}
	else if (x.size() == 2 && x[0] == "jnbe")
	{
		process(processJnbe(x, n));
		return;
	}
	else if (x.size() == 2 && x[0] == "jna")
	{
		process(processJna(x, n));
		return;
	}
	else if (x.size() == 2 && x[0] == "jnb")
	{
		process(processJnb(x, n));
		return;
	}
	else if (x.size() == 2 && x[0] == "jnc")
	{
		process(processJnc(x, n));
		return;
	}
	else if (x.size() == 2 && x[0] == "jc")
	{
		process(processJc(x, n));
		return;
	}
	else if (x.size() == 2 && x[0] == "int" && lower(x[1]) == "21h")
	{
		processInt21h(x);
	}
	else if (x.size() == 2 && x[0] == "int" && lower(x[1]) == "20h")
	{
		return;
	}
	else if (x.size() == 1 && x[0] == "nop")
	{

	}
	else if (x.size() == 3 && x[0] == "rcl")
	{
		processRcl(x);
	}
	else if (x.size() == 3 && x[0] == "rcr")
	{
		processRcr(x);
	}
	else if (x.size() == 3 && x[0] == "shl")
	{
		processShl(x);
	}
	else if (x.size() == 3 && x[0] == "shr")
	{
		processShr(x);
	}
	else
	{
		PrintError("Instruction is not valid!");
	}

	process(n + 1);
}

//todo ubuntuda derle 

int main(int count, char** args)
{
	//fcin.open("");

	if (count < 2)
		PrintError("command line arguments!");

	fcin.open(args[1]);//Read file

	memset(memory, 0, 64 * 1024);//clean the memory
	memset(registers, 0, 16); // clean the registers

	string sp = "sp";
	*read16bitAddress(sp, nullflag1) = 0xFFFE;//set SP = 0xFFFE, stack pointer

	while (readcleanline()) //Read lines until it returns false.
		;


	lines[numoflines] = writeindex;//the last line 

	process(0); // start by processing line 0
	return 0;
}


//MOV, ADD, SUB, MUL, DIV, XOR, OR, AND, NOT,
//RCL, RCR, SHL, SHR, PUSH, POP, NOP, CMP, JZ, JNZ, JE, JNE, JA, JAE, JB, JBE, JNAE, JNB,
//JNBE, JNC, JC, PUSH, POP, INT	20h
//RCL, RCR, SHL, SHR, NOP