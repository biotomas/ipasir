#include "carj.h"
#include <string>

#include "json.hpp"
#include "carj/logging.h"


INITIALIZE_EASYLOGGINGPP
using json = nlohmann::json;

const std::string carj::Carj::configPath = "carj.json";

carj::Carj& carj::getCarj() {
	static carj::Carj carj;
	return carj;
}

std::vector<carj::CarjArgBase*>& carj::getArgs(){
	static std::vector<carj::CarjArgBase*> args;
	return args;
}

void initLogger(){
	el::Configurations conf;
	conf.setToDefault();
	conf.setGlobally(el::ConfigurationType::Format, "ci %level %fbase:%line; %msg");
	conf.set(el::Level::Fatal, el::ConfigurationType::Format, "%level %fbase:%line; %msg");
	el::Loggers::reconfigureAllLoggers(conf);

	conf.setGlobally(el::ConfigurationType::Format, "ci %level %datetime{%H:%m:%s}; %msg");
	el::Loggers::reconfigureLogger("performance", conf);
	el::Loggers::setVerboseLevel(2);
}

void carj::init(int argc, const char **argv, TCLAP::CmdLine& cmd,
	std::string parameterBase) {
	START_EASYLOGGINGPP(argc, argv);
	initLogger();

	std::array<const char*, 2> argv_help = {argv[0], "-h"};
	if (argc == 1) {
		argc = 2;
		argv = argv_help.data();
	}

	TCLAP::ValueArg<std::string> configPath("", "configPath",
		/*description*/ "Path to configuartion file.",
		/*necessary*/   false,
		/*default*/     "carj.json",
		/*type descr.*/ "path");
	cmd.add(configPath);

	TCLAP::SwitchArg useConfig("", "useConfig",
		"Use configuration file 'carj.json'.",
		/*default*/ false);
	cmd.add(useConfig);

	TCLAP::ValueArg<std::string> jsonParameterBase("", "jsonParameterBase",
		"Json pointer to base of parameters.",
		/*necessary*/   false,
		/*default*/     parameterBase,
		/*description*/ "");
	cmd.add(jsonParameterBase);

	cmd.parse( argc, argv );

	carj::getCarj().init(
		configPath.getValue(),
		configPath.isSet() || useConfig.getValue(),
		jsonParameterBase.getValue());
	carj::CarjArgBase::writeAllToJson();


}