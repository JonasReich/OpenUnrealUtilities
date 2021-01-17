// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Widgets/Notifications/SNotificationList.h"

#include "SlateNotificationLibrary.generated.h"

struct FNotificationInfo;
class SNotificationItem;

DECLARE_DYNAMIC_DELEGATE(FSlateNotificationSimpleSingleCastDelegate);

USTRUCT(BlueprintType)
struct FSlateNotificationInfo
{
	GENERATED_BODY()
public:
	/** The text displayed in the notification */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slate Notification")
	FText Text;

	/** The fade in duration for the notification */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slate Notification|Timing")
	float FadeInDuration;

	/** The fade out duration for the notification */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slate Notification|Timing")
	float FadeOutDuration;

	/** The duration before a fadeout for the notification */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slate Notification|Timing")
	float ExpireDuration;

	/** When true an image is displayed next to the text */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slate Notification")
	bool bShowImage;
	
	/** The icon image to display next to the text */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slate Notification")
	FSlateBrush Image;
	
	/** Controls whether or not to add the animated throbber */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slate Notification|Styling")
	bool bUseThrobber;

	/** Controls whether or not to display the success and fail icons */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slate Notification|Styling")
	bool bUseSuccessFailIcons;

	/** When true the larger bolder font will be used to display the message */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slate Notification|Styling")
	bool bUseLargeFont;

	/** When set to a bigger value than zero this forces the width of the box, used to stop resizing on text change */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slate Notification|Styling")
	float WidthOverride;

	/** When true the notification will automatically time out after the expire duration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slate Notification|Misc")
	bool bFireAndForget;

	FNotificationInfo ToNativeNotificationInfo() const;
};

USTRUCT(BlueprintType)
struct OUURUNTIME_API FSlateNotificationHandle
{
	GENERATED_BODY()
public:
	friend OUURUNTIME_API uint32 GetTypeHash(const FSlateNotificationHandle&);

	void NewGuid();
	bool IsValid() const;

	bool operator==(const FSlateNotificationHandle& Other) const;

	static FSlateNotificationHandle InvalidHandle();

private:
	/** integer handle that is use to identify registered notifications */
	FGuid NotificationId = FGuid();	
};

OUURUNTIME_API uint32 GetTypeHash(const FSlateNotificationHandle& Handle);

UENUM(BlueprintType)
enum class ESlateNotificationState : uint8
{
	None,
	Pending,
	Success,
	Fail
};

OUURUNTIME_API ESlateNotificationState SlateNotificationState_ConvertSlateToBlueprint(SNotificationItem::ECompletionState State);
OUURUNTIME_API SNotificationItem::ECompletionState SlateNotificationState_ConvertBlueprintToSlate(ESlateNotificationState State);

UCLASS()
class USlateNotificationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	static FSlateNotificationHandle AddSlateNotification(const FSlateNotificationInfo& Info);

	UFUNCTION(BlueprintCallable)
	static void SetSlateNotificationText(const FSlateNotificationHandle& NotificationHandle, FText Text);
	UFUNCTION(BlueprintCallable)
	static void SetSlateNotificationHyperlink(const FSlateNotificationHandle& NotificationHandle, FText HyperlinkText, FSlateNotificationSimpleSingleCastDelegate HyperlinkDelegate);
	UFUNCTION(BlueprintCallable)
	static void SetSlateNotificationExpireDuration(const FSlateNotificationHandle& NotificationHandle, float ExpireDuration);
	UFUNCTION(BlueprintCallable)
	static void SetSlateNotificationFadeInDuration(const FSlateNotificationHandle& NotificationHandle, float FadeInDuration);
	UFUNCTION(BlueprintCallable)
	static void SetSlateNotificationFadeOutDuration(const FSlateNotificationHandle& NotificationHandle, float FadeOutDuration);

	UFUNCTION(BlueprintPure)
	static ESlateNotificationState GetSlateNotificationCompletionState(const FSlateNotificationHandle& NotificationHandle);
	UFUNCTION(BlueprintCallable)
	static void SetSlateNotificationCompletionState(const FSlateNotificationHandle& NotificationHandle, ESlateNotificationState CompletionState);

	UFUNCTION(BlueprintCallable)
	static void ExpireSlateNotificationAndFadeout(const FSlateNotificationHandle& NotificationHandle);
	UFUNCTION(BlueprintCallable)
	static void FadeoutSlateNotification(const FSlateNotificationHandle& NotificationHandle);
	UFUNCTION(BlueprintCallable)
	static void PulseSlateNotification(const FSlateNotificationHandle& NotificationHandle, FLinearColor GlowColor);

	UFUNCTION(BlueprintCallable)
	static void ReleaseSlateNotificationHandle(const FSlateNotificationHandle& NotificationHandle);

private:
	using FNotificationMap = TMap<FSlateNotificationHandle, TSharedPtr<SNotificationItem>>;
	FNotificationMap RegisteredNotifications;
	static FNotificationMap& GetNotificationMap();
	static SNotificationItem* FindNotification(const FSlateNotificationHandle& Handle);
};
