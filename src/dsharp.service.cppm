module;

#include <ds/service/languageService.hpp>
#include <ds/service/scannedSymbols.hpp>

export module dsharp.service;

export namespace ds
{
	export using ds::ScannedEnum;
	export using ds::ScannedFile;
	export using ds::ScannedFunction;
	export using ds::ScannedMember;
	export using ds::ScannedScope;
	export using ds::ScannedScopeVariable;
	export using ds::ScannedType;
	export using ds::ScannedTypeUsage;
	export using ds::ScannedVariable;
	export using ds::LanguageService;
	export using ds::AutoCompleteResult;
	export using ds::CompletionType;
}