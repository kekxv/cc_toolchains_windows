#pragma once

// =============================================================
// Visual Studio Setup Configuration COM interface definitions
// Shared by cl_wrapper and lib_wrapper
// =============================================================

#include <windows.h>
#include <unknwn.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "version.lib")

#ifndef __ISetupConfiguration_INTERFACE_DEFINED__
#define __ISetupConfiguration_INTERFACE_DEFINED__

const IID IID_ISetupInstance = {0xB41463C3, 0x8866, 0x43B5, {0xBC, 0x33, 0x2B, 0x06, 0x76, 0xF7, 0xF4, 0x2E}};
const IID IID_ISetupInstance2 = {0x89143C9A, 0x05AF, 0x49B0, {0xB7, 0x17, 0x72, 0xE2, 0x18, 0xA2, 0x18, 0x5C}};
const IID IID_IEnumSetupInstances = {0x6380BC6E, 0x7C73, 0x4459, {0x98, 0xC0, 0x1A, 0x31, 0x59, 0x3D, 0x29, 0x9E}};
const IID IID_ISetupConfiguration = {0x42843719, 0xDB4C, 0x46C2, {0x8E, 0x7C, 0x64, 0xF1, 0x81, 0x6D, 0xFD, 0x5B}};
const IID IID_ISetupConfiguration2 = {0x26AAB78C, 0x4A60, 0x49D6, {0xAF, 0x3B, 0x3C, 0x35, 0xBC, 0x93, 0x36, 0x5D}};
const CLSID CLSID_SetupConfiguration = {0x177F0C4A, 0x1CD3, 0x4DE7, {0xA3, 0x2C, 0x71, 0xDB, 0xBB, 0x9F, 0xA3, 0x6D}};

enum InstanceState
{
  eNone = 0, eLocal = 1, eRegistered = 2, eNoRebootRequired = 4, eNoErrors = 8, eComplete = 4294967295
};

struct ISetupInstance : public IUnknown
{
  virtual HRESULT STDMETHODCALLTYPE GetInstanceId(BSTR* pbstrInstanceId) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetInstallDate(LPFILETIME pInstallDate) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetInstallationName(BSTR* pbstrInstallationName) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetInstallationPath(BSTR* pbstrInstallationPath) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetInstallationVersion(BSTR* pbstrInstallationVersion) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetDisplayName(LCID lcid, BSTR* pbstrDisplayName) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetDescription(LCID lcid, BSTR* pbstrDescription) = 0;
  virtual HRESULT STDMETHODCALLTYPE ResolvePath(LPCOLESTR pwszRelativePath, BSTR* pbstrAbsolutePath) = 0;
};

struct ISetupInstance2 : public ISetupInstance
{
  virtual HRESULT STDMETHODCALLTYPE GetState(InstanceState* pState) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetPackages(SAFEARRAY** ppsaPackages) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetProduct(void** ppPackage) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetProductPath(BSTR* pbstrProductPath) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetErrors(void** ppErrorState) = 0;
  virtual HRESULT STDMETHODCALLTYPE IsLaunchable(VARIANT_BOOL* pfIsLaunchable) = 0;
  virtual HRESULT STDMETHODCALLTYPE IsComplete(VARIANT_BOOL* pfIsComplete) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetProperties(void** ppProperties) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetEnginePath(BSTR* pbstrEnginePath) = 0;
};

struct IEnumSetupInstances : public IUnknown
{
  virtual HRESULT STDMETHODCALLTYPE Next(ULONG celt, ISetupInstance** rgelt, ULONG* pceltFetched) = 0;
  virtual HRESULT STDMETHODCALLTYPE Skip(ULONG celt) = 0;
  virtual HRESULT STDMETHODCALLTYPE Reset(void) = 0;
  virtual HRESULT STDMETHODCALLTYPE Clone(IEnumSetupInstances** ppenum) = 0;
};

struct ISetupConfiguration : public IUnknown
{
  virtual HRESULT STDMETHODCALLTYPE EnumInstances(IEnumSetupInstances** ppEnumInstances) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetInstanceForCurrentProcess(ISetupInstance** ppInstance) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetInstanceForPath(LPCOLESTR pwszPath, ISetupInstance** ppInstance) = 0;
};

struct ISetupConfiguration2 : public ISetupConfiguration
{
  virtual HRESULT STDMETHODCALLTYPE EnumAllInstances(IEnumSetupInstances** ppEnumInstances) = 0;
};

#endif
