#pragma once
#include<iostream>
#include<fstream>
using namespace std;

class PPM
{
public:
	PPM(int width=640, int height=480)
	{
		_nWidth = width;
		_nHeight = height;
		_nChannel = 3;
		_rgb_data = new unsigned char[3*_nWidth*_nHeight];
	}

	~PPM(){
		delete []_rgb_data;
	}

	void SetPixel(int row, int col, unsigned r, unsigned g, unsigned b)
	{
		int idx = 3 * (row * _nWidth + col);
		_rgb_data[idx + 0] = r;
		_rgb_data[idx + 1] = g;
		_rgb_data[idx + 2] = b;
	}

	void Write2File(const char* filename = "output.ppm")
	{
		ofstream fp(filename);
		if (!fp)
		{
			cerr << "Failed to open output file: " << filename << endl;
			return;
		}
		fp << "P3\n" << _nWidth << " " << _nHeight << "\n255\n";
		int idx = 0;
		for (int j = _nHeight - 1; j >= 0; --j)
		{
			for (int i = 0; i < _nWidth; ++i)
			{
				idx = 3*(j * _nWidth + i);
				fp << static_cast<int>(_rgb_data[idx]) << " "
					<< static_cast<int>(_rgb_data[idx + 1]) << " "
					<< static_cast<int>(_rgb_data[idx + 2]) << "\n";
			}
		}
	}

private:
	unsigned char* _rgb_data;
	int _nHeight;
	int _nWidth;
	int _nChannel;
};
