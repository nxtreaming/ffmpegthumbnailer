#include "videothumbnailer.h"
#include "pngwriter.h"
#include "exception.h"
#include "stringoperations.h"

#include <iostream>
#include <sstream>
#include <assert.h>
#include <algorithm>
#include <sys/stat.h>

using namespace std;

VideoThumbnailer::VideoThumbnailer(const std::string& videoFile)
: m_MovieDecoder(videoFile)
, m_VideoFileName(videoFile)
{
}

VideoThumbnailer::~VideoThumbnailer()
{
}

void VideoThumbnailer::generateThumbnail(const string& outputFile, int thumbnailSize)
{
	VideoFrame 	videoFrame;
	
	//before seeking, a frame has to be decoded
	m_MovieDecoder.decodeVideoFrame();
	try
	{
		m_MovieDecoder.seek(m_MovieDecoder.getDuration() / 10);
	}
	catch (Exception& e)
	{
		cout << e.getMessage() << endl;
	}
	
	m_MovieDecoder.getScaledVideoFrame(thumbnailSize, videoFrame);
	
	vector<byte*> rowPointers;
	for (int i = 0; i < videoFrame.height; ++i)
	{
		rowPointers.push_back(&(videoFrame.frameData[i * videoFrame.lineSize])); 
	}
	
	writePng(outputFile, videoFrame, rowPointers);
}

void VideoThumbnailer::writePng(const string& outputFile, const VideoFrame& videoFrame, vector<byte*>& rowPointers)
{	
	PngWriter pngWriter(outputFile);
	
	struct stat statInfo;
    if (stat(m_VideoFileName.c_str(), &statInfo) == 0)
    {
		pngWriter.setPngText("Thumb::MTime", StringOperations::toString(statInfo.st_mtime));
		pngWriter.setPngText("Thumb::Size", StringOperations::toString(statInfo.st_size));
    }
    else
    {
    	throw Exception("Could not stat file");
    } 
    
    string mimeType = getMimeType();
    if (!mimeType.empty())
    {
    	pngWriter.setPngText("Thumb::Mimetype", mimeType);
    }
	
	pngWriter.setPngText("Thumb::URI", m_VideoFileName);
	pngWriter.setPngText("Thumb::Movie::Length", StringOperations::toString(m_MovieDecoder.getDuration()));
	pngWriter.writeFrame(&(rowPointers.front()), videoFrame.width, videoFrame.height);
}

string VideoThumbnailer::getMimeType()
{
	string extension = getExtension(m_VideoFileName);
	
	if (extension == "avi")
	{
		return "video/x-msvideo";
	}
	else if (extension == "mpeg" || extension == "mpg" || extension == "mpe" || extension == "vob")
	{
		return "video/mpeg";
	}
	else if (extension == "qt" || extension == "mov")
	{
		return "video/quicktime";
	}
	else if (extension == "asf" || extension == "asx")
	{
		return "video/x-ms-asf";
	}
	else if (extension == "wm")
	{
		return "video/x-ms-wm";
	}
	else if (extension == "mp4")
	{
		return "video/x-ms-wmv";
	}
	else if (extension == "mp4")
	{
		return "video/mp4";
	}
	else if (extension == "flv")
	{
		return "video/x-flv";
	}
	else
	{
		return "";
	}
}

string VideoThumbnailer::getExtension(const string& videoFilename)
{
	string extension;
	string::size_type pos = videoFilename.rfind('.');
	
	if (string::npos != pos)
	{
		extension = videoFilename.substr(pos + 1, videoFilename.size());
	}
	
	return extension;
}

void VideoThumbnailer::generateHistogram(const VideoFrame& videoFrame, std::map<byte, int>& histogram)
{
	for (int i = 0; i < videoFrame.height; ++i)
	{
		int pixelIndex = i * videoFrame.lineSize;
		for (int j = 0; j < videoFrame.width * 3; j += 3)
		{
			byte r = videoFrame.frameData[pixelIndex + j];
			byte g = videoFrame.frameData[pixelIndex + j + 1];
			byte b = videoFrame.frameData[pixelIndex + j + 2];
			
			byte luminance = static_cast<byte>(0.299 * r + 0.587 * g + 0.114 * b);
			histogram[luminance] += 1;
		}
	}
}

bool VideoThumbnailer::isDarkImage(const int numPixels, const Histogram& histogram)
{
	int darkPixels = 0;
	
	Histogram::const_iterator iter;
	for(iter = histogram.begin(); iter->first < 15; ++iter)
	{
		darkPixels += iter->second;
	}
	
	return darkPixels > static_cast<int>(numPixels / 2);
}
