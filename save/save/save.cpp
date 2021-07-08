#include <iostream>
#include "pch.h"
#include <string>
#include <memory>
#include <thread>

using namespace std;

AVFormatContext *inputContext = nullptr;
AVFormatContext *outputContext;

int OpenInput(string inputUrl)
{
	inputContext = avformat_alloc_context();
	int ret = avformat_open_input(&inputContext, inputUrl.c_str(), nullptr, nullptr);
	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Input file open input failed\n");
		return ret;
	}

	ret = avformat_find_stream_info(inputContext, nullptr);
	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Find input file stream inform failed\n");
	}
	else
	{
		av_log(NULL, AV_LOG_FATAL, "Open input file %s success\n", inputUrl.c_str());
	}

	return ret;
}

shared_ptr<AVPacket> ReadPacketFromSource()
{
	//shared_ptr<AVPacket> packet(static_cast<AVPacket*>(av_malloc(sizeof(AVPacket))), [&](AVPacket *p) {av_packet_free(&p); av_freep(&p); });
	shared_ptr<AVPacket> packet(static_cast<AVPacket*>(av_malloc(sizeof(AVPacket))), [&](AVPacket *p) {av_packet_free(&p); });
	av_init_packet(packet.get());
	int ret = av_read_frame(inputContext, packet.get());
	if (ret >= 0)
	{
		return packet;
	}
	else
	{
		return nullptr;
	}
}

void av_packet_rescale_ts(AVPacket *pkt, AVRational src_tb, AVRational dst_tb)
{
	if (pkt->pts != AV_NOPTS_VALUE)
		pkt->pts = av_rescale_q(pkt->pts, src_tb, dst_tb);
	if (pkt->dts != AV_NOPTS_VALUE)
		pkt->dts = av_rescale_q(pkt->dts, src_tb, dst_tb);
	if (pkt->duration > 0)
		pkt->duration = av_rescale_q(pkt->duration, src_tb, dst_tb);
}

int WritePacket(shared_ptr<AVPacket> packet)
{
	auto inputStream = inputContext->streams[packet->stream_index];
	auto outputStream = outputContext->streams[packet->stream_index];
	av_packet_rescale_ts(packet.get(), inputStream->time_base, outputStream->time_base);
	return av_interleaved_write_frame(outputContext, packet.get());
}

int OpenOutput(string outUrl)
{
	int ret = avformat_alloc_output_context2(&outputContext, nullptr, "mpegts", outUrl.c_str());
	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "open output context failed\n");
		goto Error;
	}

	ret = avio_open2(&outputContext->pb, outUrl.c_str(), AVIO_FLAG_READ_WRITE, nullptr, nullptr);
	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "open avio failed");
		goto Error;
	}

	for (int i = 0; i<inputContext->nb_streams; i++)
	{
		AVStream *stream = avformat_new_stream(outputContext, inputContext->streams[i]->codec->codec);
		ret = avcodec_copy_context(stream->codec, inputContext->streams[i]->codec);
		if (ret < 0)
		{
			av_log(NULL, AV_LOG_ERROR, "copy codec context failed");
			goto Error;
		}
	}

	ret = avformat_write_header(outputContext, nullptr);
	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "format write header failed");
		goto Error;
	}

	av_log(NULL, AV_LOG_FATAL, "open output file success %s\n");
	return ret;
Error:
	if (outputContext)
	{
		for (int i = 0; i < outputContext->nb_streams; i++)
		{
			avcodec_close(outputContext->streams[i]->codec);
		}
		avformat_close_input(&outputContext);
	}
	return ret;
}


void Init()
{
	av_register_all();
	avfilter_register_all();
	avformat_network_init();
	av_log_set_level(AV_LOG_ERROR);
}
int main()
{
	Init();
	int ret = OpenInput("rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov");
	if (ret >= 0)
	{
		ret = OpenOutput("D:\\test1.ts");
	}
	if (ret < 0)
		goto Error;
	while (true)
	{
		auto packet = ReadPacketFromSource();
		if (packet)
		{
			ret = WritePacket(packet);
			if (ret >= 0)
			{
				cout << "WritePacket success" << endl;
			}
			else
			{
				cout << "WritePacket failed!" << endl;
			}
		}
	}
Error:
	while (1)
	{
		this_thread::sleep_for(chrono::seconds(100));
	}
	return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
