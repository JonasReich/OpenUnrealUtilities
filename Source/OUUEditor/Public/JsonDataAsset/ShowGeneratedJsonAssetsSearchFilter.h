// Copyright (c) 2023 Jonas Reich & Contributors

#include "CoreMinimal.h"

#include "ContentBrowserFrontEndFilterExtension.h"

#include "ShowGeneratedJsonAssetsSearchFilter.generated.h"

UCLASS()
class UShowGeneratedJsonAssetsSearchFilter : public UContentBrowserFrontEndFilterExtension
{
public:
	GENERATED_BODY()

	// - UContentBrowserFrontEndFilterExtension
	virtual void AddFrontEndFilterExtensions(
		TSharedPtr<class FFrontendFilterCategory> DefaultCategory,
		TArray<TSharedRef<class FFrontendFilter>>& InOutFilterList) const override;
	// --
};
