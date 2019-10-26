#include "pch.h"
#include "argagg.h"
#include <iostream>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Media.Ocr.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.Data.Pdf.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <Windows.h>
#include <shellapi.h>



using namespace winrt;

// This example code shows how you could implement the required main function
// for a Console UWP Application. You can replace all the code inside main
// with your own custom code.

// You should also change the Alias value in the AppExecutionAlias Extension
// in the package.appxmanifest to a value that you define. To edit this file
// manually, open it with the XML Editor.
std::optional<const winrt::Windows::Globalization::Language> getlangbytag(std::wstring tag) {
	if (tag.empty()) {
		return {};
	}
	return winrt::Windows::Globalization::Language(tag);
	//auto s = tag.c_str();
	//for (const auto one : winrt::Windows::Media::Ocr::OcrEngine::AvailableRecognizerLanguages()) {
	//	if (wcscmp(one.LanguageTag().c_str(), s)==1) {
	//		//std::wcout << "try to select" << one.LanguageTag().c_str() << std::endl;
	//		return one;
	//	}
	//}
	//return {};
}
int wmain()
{
    setlocale(LC_ALL, "");
	SetConsoleCP(1200);
	std::list<std::wstring> list;
    for (int i = 0; i < __argc; i++)
    {
		list.push_back(__wargv[i]);
    }
	argagg::parser argparser{ {
	{ L"help", {L"-h", L"--help"},
	  L"shows this help message", 0},
	{ L"input", {L"-i", L"--input"},
	  L"pdf file", 1},
	{ L"output", {L"-o", L"--output"},
	  L"txt file dir", 1},
	{ L"outputname", {L"-n", L"--outputname"},
	 L"txt file name", 1},
	{ L"lang", {L"-l", L"--lang"},
	  L"file language", 1},
	{ L"listlang", {L"-s", L"--listlang"},
	  L"show support languages", 0},
  } };
	std::cout << "website: https://github.com/yinyue200/PDFtoTXT" << std::endl;
	try {
		if (list.empty()) {
			std::wcerr << argparser;
		}
		else {
			const auto&& result = argparser.parse(__argc, __wargv);
			if (result[L"listlang"]) {
				for (const auto&& one : winrt::Windows::Media::Ocr::OcrEngine::AvailableRecognizerLanguages()) {
					std::wcout << (one.DisplayName() + winrt::hstring(L" (") + one.LanguageTag() + winrt::hstring(L")")).c_str() << std::endl;
				}
			}
			else if (result[L"input"]&& result[L"output"]) {
				const auto&& inputpath = result[L"input"].as<std::wstring>();
				const auto&& outputpath = result[L"output"].as<std::wstring>();
				const auto&& outputpathname = result[L"outputname"].as<std::wstring>();
				const auto&& langtag = result[L"listlang"].as<std::wstring>(L"");
				auto lang = getlangbytag(langtag);
				winrt::Windows::Media::Ocr::OcrEngine engine(lang.has_value() ? winrt::Windows::Media::Ocr::OcrEngine::TryCreateFromLanguage(lang.value()) : winrt::Windows::Media::Ocr::OcrEngine::TryCreateFromUserProfileLanguages());
				std::wcout << engine.RecognizerLanguage().LanguageTag().c_str() << std::endl;
				//if (engine == nullptr) {
				//	std::cerr << "Does not support parsing and display of current language, please manually select the language" << std::endl;
				//	return EXIT_FAILURE;
				//}
				const auto&& inputfile = winrt::Windows::Storage::StorageFile::GetFileFromPathAsync(inputpath).get();
				const auto&& outputfolder = winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(outputpath).get();
				const auto&& outputfile = outputfolder.CreateFileAsync(outputpathname, winrt::Windows::Storage::CreationCollisionOption::ReplaceExisting).get();
				const auto&& pd = winrt::Windows::Data::Pdf::PdfDocument::LoadFromFileAsync(inputfile).get();
				int barWidth = 70;
				if (pd.IsPasswordProtected()) {
					std::cerr << "Unable to read PDF files with a password" << std::endl;
					return EXIT_FAILURE;
				}

				{
					auto&& stream = outputfile.OpenAsync(winrt::Windows::Storage::FileAccessMode::ReadWrite).get();
					winrt::Windows::Storage::Streams::DataWriter writer(stream);
					auto maxpage = pd.PageCount();
					for (auto i = 0u; i < maxpage; i++)
					{
						auto&& page = pd.GetPage(i);
						page.PreparePageAsync().get();
						auto str = winrt::Windows::Storage::Streams::InMemoryRandomAccessStream();
						page.RenderToStreamAsync(str).get();


						{
							std::cout << "[";
							auto progress = ((i+1) / maxpage);
							int pos = barWidth * progress;
							for (int i = 0; i < barWidth; ++i) {
								if (i < pos) std::cout << "=";
								else if (i == pos) std::cout << ">";
								else std::cout << " ";
							}
							std::cout << "] " << int(progress * 100.0) << " %\r";
							std::cout.flush();
						}

						auto&& decoder = winrt::Windows::Graphics::Imaging::BitmapDecoder::CreateAsync(str).get();
						auto&& re = engine.RecognizeAsync(decoder.GetSoftwareBitmapAsync().get()).get();
						//auto text = re.Text().c_str();
						writer.WriteString(re.Text());
					}
					writer.FlushAsync().get();
					writer.StoreAsync().get();
					writer.Close();
					stream.Close();
					
					std::cout << "completed" << std::endl;
				}


			}
			else {
				std::wcerr << argparser;
				getchar();
			}
		}
	}
	catch (const winrt::hresult_access_denied& e) {
		std::cerr << "Unable to read you file. Please visit https://support.microsoft.com/help/4468237/windows-10-file-system-access-and-privacy-microsoft-privacy for help" << std::endl;
		return EXIT_FAILURE;
	}
	catch (const winrt::hresult_invalid_argument & e) {
		std::cerr << "Invalid argument" << std::endl;
		return EXIT_FAILURE;
	}
	catch (const std::exception & e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

