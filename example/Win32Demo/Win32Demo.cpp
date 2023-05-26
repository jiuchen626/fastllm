﻿#include <cstdio>
#include <cstring>
#include <iostream>

#include "StringUtils.h"
#include "factoryllm.h"

static factoryllm fllm;
static int modeltype = 0;
static char* modelpath = NULL;
static fastllm::basellm* chatGlm = fllm.createllm(LLM_TYPE_CHATGLM);
static fastllm::basellm* moss = fllm.createllm(LLM_TYPE_MOSS);
static int sRound = 0;
static std::string history;

struct RunConfig {
	int model = LLM_TYPE_CHATGLM; // 模型类型, 0 chatglm,1 moss,2 alpaca 参考LLM_TYPE
	std::string path = "C:\\huqb\\AI\\ChatGLM\\chatglm-6b-int4.bin"; // 模型文件路径
	int threads = 4; // 使用的线程数
	bool lowMemMode = false; // 是否使用低内存模式
};

void Usage() {
	std::cout << "Usage:" << std::endl;
	std::cout << "[-h|--help]:                      显示帮助" << std::endl;
	std::cout << "<-m|--model> <args>:              模型类型，默认为0, 可以设置为0(chatglm),1(moss),2(alpaca)" << std::endl;
	std::cout << "<-p|--path> <args>:               模型文件的路径" << std::endl;
	std::cout << "<-t|--threads> <args>:            使用的线程数量" << std::endl;
	std::cout << "<-l|--low> <args>:				使用低内存模式" << std::endl;
}

void ParseArgs(int argc, char **argv, RunConfig &config) {
	std::vector <std::string> sargv;
	for (int i = 0; i < argc; i++) {
		sargv.push_back(std::string(argv[i]));
	}
	for (int i = 1; i < argc; i++) {
		if (sargv[i] == "-h" || sargv[i] == "--help") {
			Usage();
			exit(0);
		}
		else if (sargv[i] == "-m" || sargv[i] == "--model") {
			config.model = atoi(sargv[++i].c_str());
		}
		else if (sargv[i] == "-p" || sargv[i] == "--path") {
			config.path = sargv[++i];
		}
		else if (sargv[i] == "-t" || sargv[i] == "--threads") {
			config.threads = atoi(sargv[++i].c_str());
		}
		else if (sargv[i] == "-l" || sargv[i] == "--low") {
			config.lowMemMode = true;
		}
		else {
			Usage();
			exit(-1);
		}
	}
}

int initLLMConf(int model, bool isLowMem, const char* modelPath, int threads) {
	fastllm::SetThreads(threads);
	fastllm::SetLowMemMode(isLowMem);
	modeltype = model;
	//printf("@@init llm:type:%d,path:%s\n", model, modelPath);
	if (modeltype == 0) {
		chatGlm->LoadFromFile(modelPath);
	}
	if (modeltype == 1) {
		moss->LoadFromFile(modelPath);
	}
	return 0;
}

int chat(const char* prompt) {
	std::string ret = "";
	//printf("@@init llm:type:%d,prompt:%s\n", modeltype, prompt);
	std::string input(prompt);
	if (modeltype == LLM_TYPE_CHATGLM) {
		if (input == "reset") {
			history = "";
			sRound = 0;
			return 0;
		}
		history += ("[Round " + std::to_string(sRound++) + "]\n问：" + input);
		auto prompt = sRound > 1 ? history : input;
		ret = chatGlm->Response(Gb2utf(prompt), [](int index, const char* content) {
			std::string result = utf2Gb(content);
			if (index == 0) {
				printf("ChatGLM:%s", result.c_str());
			}
			if (index > 0) {
				printf("%s", result.c_str());
			}
			if (index == -1) {
				printf("\n");
			}

		});
		history += ("\n答：" + ret + "\n");
	}

	if (modeltype == LLM_TYPE_MOSS) {
		auto prompt = "You are an AI assistant whose name is MOSS. <|Human|>: " + Gb2utf(input) + "<eoh>";
		ret = moss->Response(prompt, [](int index, const char* content) {
			std::string result = utf2Gb(content);
			if (index == 0) {
				printf("MOSS:%s", result.c_str());
			}
			if (index > 0) {
				printf("%s", result.c_str());
			}
			if (index == -1) {
				printf("\n");
			}
		});
	}
	long len = ret.length();
	return len;
}

void uninitLLM()
{
	if (chatGlm)
	{
		delete chatGlm;
		chatGlm = NULL;
	}
	if (moss)
	{
		delete moss;
		moss = NULL;
	}
}

int main(int argc, char **argv) {
	RunConfig config;
	ParseArgs(argc, argv, config);
	initLLMConf(config.model, config.lowMemMode, config.path.c_str(), config.threads);

	if (config.model == LLM_TYPE_MOSS) {

		while (true) {
			printf("用户: ");
			std::string input;
			std::getline(std::cin, input);
			if (input == "stop") {
				break;
			}
			chat(input.c_str());
		}
	}
	else if (config.model == LLM_TYPE_CHATGLM) {
		while (true) {
			printf("用户: ");
			std::string input;
			std::getline(std::cin, input);
			if (input == "stop") {
				break;
			}
			chat(input.c_str());
		}

	}
	else {
		Usage();
		exit(-1);
	}

	return 0;
}