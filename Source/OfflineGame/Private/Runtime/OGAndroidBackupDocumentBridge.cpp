#include "Runtime/OGAndroidBackupDocumentBridge.h"
#include "HAL/FileManager.h"

#if PLATFORM_ANDROID
#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"
#endif

namespace
{
#if PLATFORM_ANDROID
bool CallPickerMethod(
    const char* MethodName,
    const char* Signature,
    const TArray<FString>& Arguments,
    FString& OutError)
{
    JNIEnv* Env = FAndroidApplication::GetJavaEnv();
    if (!Env)
    {
        OutError = TEXT("Android Java environment is unavailable.");
        return false;
    }

    jmethodID Method = FJavaWrapper::FindMethod(
        Env,
        FJavaWrapper::GameActivityClassID,
        MethodName,
        Signature,
        false);
    if (!Method)
    {
        OutError = TEXT("Android backup document bridge method is unavailable.");
        return false;
    }

    if (Arguments.IsEmpty())
    {
        FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, Method);
        return true;
    }

    if (Arguments.Num() == 1)
    {
        jstring A0 = Env->NewStringUTF(TCHAR_TO_UTF8(*Arguments[0]));
        FJavaWrapper::CallVoidMethod(
            Env,
            FJavaWrapper::GameActivityThis,
            Method,
            A0);
        Env->DeleteLocalRef(A0);
        return true;
    }

    if (Arguments.Num() == 2)
    {
        jstring A0 = Env->NewStringUTF(TCHAR_TO_UTF8(*Arguments[0]));
        jstring A1 = Env->NewStringUTF(TCHAR_TO_UTF8(*Arguments[1]));
        FJavaWrapper::CallVoidMethod(
            Env,
            FJavaWrapper::GameActivityThis,
            Method,
            A0,
            A1);
        Env->DeleteLocalRef(A0);
        Env->DeleteLocalRef(A1);
        return true;
    }

    OutError = TEXT("Unsupported Android backup bridge argument count.");
    return false;
}
#endif
}

bool FOGAndroidBackupDocumentBridge::LaunchExport(
    const FString& SourceBackupPath,
    const FString& SuggestedFileName,
    FString& OutError)
{
    OutError.Reset();
#if PLATFORM_ANDROID
    if (SourceBackupPath.IsEmpty() ||
        SuggestedFileName.IsEmpty())
    {
        OutError = TEXT("Backup export source/name is empty.");
        return false;
    }

    return CallPickerMethod(
        "AndroidThunkJava_OGExportBackup",
        "(Ljava/lang/String;Ljava/lang/String;)V",
        { IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*SourceBackupPath), SuggestedFileName },
        OutError);
#else
    OutError = TEXT("External document export is available on Android.");
    return false;
#endif
}

bool FOGAndroidBackupDocumentBridge::LaunchImport(
    const FString& InternalDestinationPath,
    FString& OutError)
{
    OutError.Reset();
#if PLATFORM_ANDROID
    if (InternalDestinationPath.IsEmpty())
    {
        OutError = TEXT("Backup import destination is empty.");
        return false;
    }

    return CallPickerMethod(
        "AndroidThunkJava_OGImportBackup",
        "(Ljava/lang/String;)V",
        { IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*InternalDestinationPath) },
        OutError);
#else
    OutError = TEXT("External document import is available on Android.");
    return false;
#endif
}

FName FOGAndroidBackupDocumentBridge::GetTransferState(
    FString& OutDetail)
{
    OutDetail.Reset();
#if PLATFORM_ANDROID
    JNIEnv* Env = FAndroidApplication::GetJavaEnv();
    if (!Env)
    {
        return FName(TEXT("error"));
    }

    jmethodID Method = FJavaWrapper::FindMethod(
        Env,
        FJavaWrapper::GameActivityClassID,
        "AndroidThunkJava_OGGetBackupTransferState",
        "()Ljava/lang/String;",
        false);
    if (!Method)
    {
        return FName(TEXT("error"));
    }

    jstring Result = static_cast<jstring>(
        FJavaWrapper::CallObjectMethod(
            Env,
            FJavaWrapper::GameActivityThis,
            Method));
    if (!Result)
    {
        return FName(TEXT("idle"));
    }

    const char* Utf = Env->GetStringUTFChars(Result, nullptr);
    FString Combined = UTF8_TO_TCHAR(Utf ? Utf : "");
    if (Utf)
    {
        Env->ReleaseStringUTFChars(Result, Utf);
    }
    Env->DeleteLocalRef(Result);

    FString State;
    if (!Combined.Split(TEXT("|"), &State, &OutDetail))
    {
        State = Combined;
    }
    return State.IsEmpty()
        ? FName(TEXT("idle"))
        : FName(*State);
#else
    return FName(TEXT("unsupported"));
#endif
}

bool FOGAndroidBackupDocumentBridge::ChooseAutomaticBackupFolder(FString& OutError)
{
    OutError.Reset();
#if PLATFORM_ANDROID
    return CallPickerMethod("AndroidThunkJava_OGChooseBackupFolder", "()V", {}, OutError);
#else
    OutError = TEXT("Android backup folders are available on Android.");
    return false;
#endif
}

bool FOGAndroidBackupDocumentBridge::ShouldQueueAutomaticBackup()
{
#if PLATFORM_ANDROID
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        const jmethodID Method = FJavaWrapper::FindMethod(
            Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_OGShouldQueueAutomaticBackup", "()Z", false);
        return Method && Env->CallBooleanMethod(FJavaWrapper::GameActivityThis, Method) == JNI_TRUE;
    }
#endif
    return false;
}

bool FOGAndroidBackupDocumentBridge::QueueAutomaticBackup(
    const FString& SourcePath, const FString& Name, FString& OutError)
{
    OutError.Reset();
#if PLATFORM_ANDROID
    return CallPickerMethod("AndroidThunkJava_OGQueueAutomaticBackup",
        "(Ljava/lang/String;Ljava/lang/String;)V",
        { IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*SourcePath), Name }, OutError);
#else
    OutError = TEXT("Android backup folders are available on Android.");
    return false;
#endif
}

FName FOGAndroidBackupDocumentBridge::GetAutomaticBackupState()
{
#if PLATFORM_ANDROID
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        const jmethodID Method = FJavaWrapper::FindMethod(
            Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_OGGetAutomaticBackupState",
            "()Ljava/lang/String;", false);
        if (Method)
        {
            const jstring Result = static_cast<jstring>(
                FJavaWrapper::CallObjectMethod(Env, FJavaWrapper::GameActivityThis, Method));
            if (Result)
            {
                const char* Utf = Env->GetStringUTFChars(Result, nullptr);
                const FName State(Utf ? UTF8_TO_TCHAR(Utf) : TEXT("no_folder"));
                if (Utf)
                {
                    Env->ReleaseStringUTFChars(Result, Utf);
                }
                Env->DeleteLocalRef(Result);
                return State;
            }
        }
    }
    return TEXT("no_folder");
#else
    return TEXT("unsupported");
#endif
}
