#include "PackerMain.h"

#include <exception>
#include "Vector.h"

#include "FormatBase.h"
#include "PEFormat.h"
#include "Signature.h"
#include "Win32Loader.h"

PackerMain::PackerMain(const Option &option) : option_(option)
{
}

int PackerMain::process()
{
	for(SharedPtr<File> &file : option_.getInputFiles())
	{
		try
		{
			processFile(file);
		}
		catch(...)
		{

		}
	}

	return 0;
}

List<SharedPtr<FormatBase>> PackerMain::loadImport(SharedPtr<FormatBase> input)
{
	List<SharedPtr<FormatBase>> result;
	for(auto &i : input->getImports())
	{
		bool alreadyLoaded = false;
		String fileName(i.libraryName.get());
		for(auto &j : loadedFiles_)
			if(j == fileName)
			{
				alreadyLoaded = true;
				break;
			}
		if(alreadyLoaded)
			continue;

		if(input->isSystemLibrary(fileName))
			continue;
		SharedPtr<FormatBase> import = input->loadImport(fileName);
		loadedFiles_.push_back(import->getFilename());
		result.push_back(import);

		List<SharedPtr<FormatBase>> dependencies = loadImport(import);
		result.insert(result.end(), dependencies.begin(), dependencies.end());
	}

	return result;
}

void PackerMain::processFile(SharedPtr<File> file)
{
	SharedPtr<FormatBase> input;
	uint8_t *fileData = file->map();
	if(*(reinterpret_cast<uint16_t *>(fileData)) == IMAGE_DOS_SIGNATURE)
		input = MakeShared<PEFormat>(fileData, file->getFileName(), file->getFilePath());
	else
		throw std::exception();

	loadedFiles_.push_back(input->getFilename());
	List<SharedPtr<FormatBase>> imports = loadImport(input);
	
	//test
	Image image = input->serialize();
	List<Image> importImages;
	for(auto &i : imports)
		importImages.push_back(i->serialize());
	Win32Loader loader(image, containerToDataStorage(std::move(importImages)));
	loader.execute();
}

#if !defined(_UNICODE) || !defined(_WIN32)
int main(int argc, char **argv)
{
	return PackerMain(Option(argc, argv)).process();
}
#else
int wmain(int argc, wchar_t **argv)
{
	return PackerMain(Option(argc, argv)).process();
}
#endif
