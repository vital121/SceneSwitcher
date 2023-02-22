#include "paramerter-wrappers.hpp"

bool PatternMatchParameters::Save(obs_data_t *obj) const
{
	auto data = obs_data_create();
	obs_data_set_bool(data, "useForChangedCheck", useForChangedCheck);
	obs_data_set_double(data, "threshold", threshold);
	obs_data_set_bool(data, "useAlphaAsMask", useAlphaAsMask);
	obs_data_set_int(data, "matchMode", matchMode);
	obs_data_set_obj(obj, "patternMatchData", data);
	obs_data_release(data);
	return true;
}

bool PatternMatchParameters::Load(obs_data_t *obj)
{
	// TODO: Remove this fallback in a future version
	if (!obs_data_has_user_value(obj, "patternMatchData")) {
		useForChangedCheck =
			obs_data_get_bool(obj, "usePatternForChangedCheck");
		threshold = obs_data_get_double(obj, "threshold");
		useAlphaAsMask = obs_data_get_bool(obj, "useAlphaAsMask");
		return true;
	}
	auto data = obs_data_get_obj(obj, "patternMatchData");
	useForChangedCheck = obs_data_get_bool(data, "useForChangedCheck");
	threshold = obs_data_get_double(data, "threshold");
	useAlphaAsMask = obs_data_get_bool(data, "useAlphaAsMask");
	// TODO: Remove this fallback in a future version
	if (!obs_data_has_user_value(obj, "matchMode")) {
		matchMode = cv::TM_CCORR_NORMED;
	} else {
		matchMode = static_cast<cv::TemplateMatchModes>(
			obs_data_get_int(data, "matchMode"));
	}
	obs_data_release(data);
	return true;
}

bool ObjDetectParameters::Save(obs_data_t *obj) const
{
	auto data = obs_data_create();
	obs_data_set_string(data, "modelPath", modelPath.c_str());
	obs_data_set_double(data, "scaleFactor", scaleFactor);
	obs_data_set_int(data, "minNeighbors", minNeighbors);
	minSize.Save(data, "minSize");
	maxSize.Save(data, "maxSize");
	obs_data_set_obj(obj, "objectMatchData", data);
	obs_data_release(data);
	return true;
}

bool isScaleFactorValid(double scaleFactor)
{
	return scaleFactor > 1.;
}

bool isMinNeighborsValid(int minNeighbors)
{
	return minNeighbors >= minMinNeighbors &&
	       minNeighbors <= maxMinNeighbors;
}

bool ObjDetectParameters::Load(obs_data_t *obj)
{
	// TODO: Remove this fallback in a future version
	if (!obs_data_has_user_value(obj, "patternMatchData")) {
		modelPath = obs_data_get_string(obj, "modelDataPath");
		scaleFactor = obs_data_get_double(obj, "scaleFactor");
		if (!isScaleFactorValid(scaleFactor)) {
			scaleFactor = 1.1;
		}
		minNeighbors = obs_data_get_int(obj, "minNeighbors");
		if (!isMinNeighborsValid(minNeighbors)) {
			minNeighbors = minMinNeighbors;
		}
		minSize.Load(obj, "minSize");
		maxSize.Load(obj, "maxSize");
		return true;
	}
	auto data = obs_data_get_obj(obj, "objectMatchData");
	modelPath = obs_data_get_string(data, "modelPath");
	scaleFactor = obs_data_get_double(data, "scaleFactor");
	if (!isScaleFactorValid(scaleFactor)) {
		scaleFactor = 1.1;
	}
	minNeighbors = obs_data_get_int(data, "minNeighbors");
	if (!isMinNeighborsValid(minNeighbors)) {
		minNeighbors = minMinNeighbors;
	}
	minSize.Load(data, "minSize");
	maxSize.Load(data, "maxSize");
	obs_data_release(data);
	return true;
}

bool AreaParamters::Save(obs_data_t *obj) const
{
	auto data = obs_data_create();
	obs_data_set_bool(data, "enabled", enable);
	area.Save(data, "area");
	obs_data_set_obj(obj, "areaData", data);
	obs_data_release(data);
	return true;
}

bool AreaParamters::Load(obs_data_t *obj)
{
	// TODO: Remove this fallback in a future version
	if (!obs_data_has_user_value(obj, "areaData")) {
		enable = obs_data_get_bool(obj, "checkAreaEnabled");
		area.Load(obj, "checkArea");
		return true;
	}
	auto data = obs_data_get_obj(obj, "areaData");
	enable = obs_data_get_bool(data, "enabled");
	area.Load(data, "area");
	obs_data_release(data);
	return true;
}

bool VideoInput::Save(obs_data_t *obj) const
{
	auto data = obs_data_create();
	obs_data_set_int(data, "type", static_cast<int>(type));
	source.Save(data);
	scene.Save(data);
	obs_data_set_obj(obj, "videoInputData", data);
	obs_data_release(data);
	return true;
}

bool VideoInput::Load(obs_data_t *obj)
{
	// TODO: Remove this fallback in a future version
	if (obs_data_has_user_value(obj, "videoType")) {
		enum class VideoSelectionType {
			SOURCE,
			OBS_MAIN,
		};
		auto oldType = static_cast<VideoSelectionType>(
			obs_data_get_int(obj, "videoType"));
		if (oldType == VideoSelectionType::SOURCE) {
			type = Type::SOURCE;
			auto name = obs_data_get_string(obj, "video");
			source.SetSource(GetWeakSourceByName(name));
		} else {
			type = Type::OBS_MAIN_OUTPUT;
		}
		return true;
	}

	auto data = obs_data_get_obj(obj, "videoInputData");
	type = static_cast<Type>(obs_data_get_int(data, "type"));
	source.Load(data);
	scene.Load(data);
	obs_data_release(data);
	return true;
}

std::string VideoInput::ToString(bool resolve) const
{
	switch (type) {
	case VideoInput::Type::OBS_MAIN_OUTPUT:
		return obs_module_text("AdvSceneSwitcher.OBSVideoOutput");
	case VideoInput::Type::SOURCE:
		return source.ToString(resolve);
	case VideoInput::Type::SCENE:
		return scene.ToString(resolve);
	}
	return "";
}

bool VideoInput::ValidSelection() const
{
	switch (type) {
	case VideoInput::Type::OBS_MAIN_OUTPUT:
		return true;
	case VideoInput::Type::SOURCE:
		return !!source.GetSource();
	case VideoInput::Type::SCENE:
		return !!scene.GetScene();
	}
	return false;
}

OBSWeakSource VideoInput::GetVideo() const
{
	switch (type) {
	case VideoInput::Type::OBS_MAIN_OUTPUT:
		return nullptr;
	case VideoInput::Type::SOURCE:
		return source.GetSource();
	case VideoInput::Type::SCENE:
		return scene.GetScene();
	}
	return nullptr;
}

static void SaveColor(obs_data_t *obj, const char *name, const QColor &color)
{
	auto data = obs_data_create();
	obs_data_set_int(data, "red", color.red());
	obs_data_set_int(data, "green", color.green());
	obs_data_set_int(data, "blue", color.blue());
	obs_data_set_obj(obj, name, data);
	obs_data_release(data);
}

OCRParameters::OCRParameters()
{
	Setup();
}

OCRParameters::~OCRParameters()
{
	if (!initDone) {
		return;
	}
	ocr->End();
}

OCRParameters::OCRParameters(const OCRParameters &other)
	: text(other.text),
	  regex(other.regex),
	  color(other.color),
	  pageSegMode(other.pageSegMode)
{
	Setup();
	if (initDone) {
		ocr->SetPageSegMode(pageSegMode);
	}
}

OCRParameters &OCRParameters::operator=(const OCRParameters &other)
{
	text = other.text;
	regex = other.regex;
	color = other.color;
	pageSegMode = other.pageSegMode;
	ocr->SetPageSegMode(pageSegMode);
	return *this;
}

bool OCRParameters::Save(obs_data_t *obj) const
{
	auto data = obs_data_create();
	text.Save(data, "pattern");
	regex.Save(data);
	SaveColor(data, "textColor", color);
	obs_data_set_int(data, "pageSegMode", static_cast<int>(pageSegMode));
	obs_data_set_obj(obj, "ocrData", data);
	obs_data_release(data);
	return true;
}

static QColor LoadColor(obs_data_t *obj, const char *name)
{
	QColor color = Qt::black;
	auto data = obs_data_get_obj(obj, name);
	color.setRed(obs_data_get_int(data, "red"));
	color.setGreen(obs_data_get_int(data, "green"));
	color.setBlue(obs_data_get_int(data, "blue"));
	obs_data_release(data);
	return color;
}

bool OCRParameters::Load(obs_data_t *obj)
{
	auto data = obs_data_get_obj(obj, "ocrData");
	text.Load(data, "pattern");
	regex.Load(data);
	color = LoadColor(data, "textColor");
	pageSegMode = static_cast<tesseract::PageSegMode>(
		obs_data_get_int(data, "pageSegMode"));
	obs_data_release(data);

	if (initDone) {
		ocr->SetPageSegMode(pageSegMode);
	}
	return true;
}

void OCRParameters::SetPageMode(tesseract::PageSegMode mode)
{
	pageSegMode = mode;
	ocr->SetPageSegMode(mode);
}

void OCRParameters::Setup()
{
	ocr = std::make_unique<tesseract::TessBaseAPI>();
	if (!ocr) {
		initDone = false;
		return;
	}
	std::string dataPath = obs_get_module_data_path(obs_current_module()) +
			       std::string("/res/ocr");
	if (ocr->Init(dataPath.c_str(), "eng") != 0) {
		initDone = false;
		return;
	}
	initDone = true;
}