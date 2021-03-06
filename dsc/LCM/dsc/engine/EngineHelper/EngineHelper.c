/*
**==============================================================================
**
** Open Management Infrastructure (OMI)
**
** Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
**
** Licensed under the Apache License, Version 2.0 (the "License"); you may not
** use this file except in compliance with the License. You may obtain a copy
** of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
** KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
** WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
** MERCHANTABLITY OR NON-INFRINGEMENT.
**
** See the Apache 2 License for the specific language governing permissions
** and limitations under the License.
**
**==============================================================================
*/

#include <MI.h>
#include "EngineHelper.h"
#include "DSC_Systemcalls.h"
#include "Resources_LCM.h"
#include "EventWrapper.h"

#if defined(_MSC_VER)
#include <windows.h>
#include "Win32_EngineHelper.h"
#include <objbase.h>
#include <winsqm.h>
#endif

extern Loc_Mapping g_LocMappingTable[];
extern MI_Uint32 g_LocMappingTableSize;

    
BaseResourceConfiguration g_BaseResourceConfiguration[] =
{
    {MI_T("ResourceId"),         MI_STRING},
    {MI_T("SourceInfo"),         MI_STRING},
    {MI_T("DependsOn"),          MI_STRINGA},
    {MI_T("ModuleName"),         MI_STRING},
    {MI_T("ModuleVersion"),      MI_STRING},        
    {NULL,                  0}
};


const MI_Char * GetSchemaSearchPath()
{    
    return CONFIGURATION_SCHEMA_SEARCH_PATH;
}
const MI_Char * GetSchemaSearchPathProgFiles()
{    
    return CONFIGURATION_PROGFILES_SCHEMA_SEARCH_PATH;
}
const MI_Char * GetRegistrationInstanceSearchPath()
{
    return CONFIGURATION_REGINSTANCE_SEARCH_PATH;
}
const MI_Char * GetRegistrationInstanceSearchPathProgFiles()
{
    return CONFIGURATION_PROGFILES_REGINSTANCE_SEARCH_PATH;
}
const MI_Char * GetCoreSchemaPath()
{
    return CONFIGURATION_BASESCHEMA_MOF_PATH;
}


JobInformation g_JobInformation={EMPTY_STRING};

void SetJobDeviceName()
{
    if (GetComputerHostName(g_JobInformation.deviceName, (int)DEVICE_NAME_SIZE) != 0) 
    {
            Stprintf(g_JobInformation.deviceName, DEVICE_NAME_SIZE, MI_T("%T"), EMPTY_STRING);
            return;
    }
   
}

MI_Result AppendWMIError1Param(
                        _Inout_ MI_Instance *cimErrorDetails,
                        _In_z_ const MI_Char * pszFormat,
                        _In_z_ const MI_Char * param1
                        )
{
    MI_Result r = MI_RESULT_OK;
    MI_Char *message = NULL;
    MI_Value value;

    r = DSC_MI_Instance_GetElement(cimErrorDetails, MSFT_WMIERROR_MESSAGE, &value, NULL, NULL, NULL);
    if( r == MI_RESULT_OK )
    {
        size_t msgLen = Tcslen(value.string) + Tcslen(param1) + Tcslen(pszFormat) + 1; 
        message = (MI_Char*) DSC_malloc(sizeof(MI_Char) *msgLen, NitsMakeCallSite(-3, NULL, NULL, 0));
        if(message)
        {
            if( Stprintf(message, msgLen, pszFormat, value.string, param1))
            {
                value.string = message;
                r = MI_Instance_SetElement(cimErrorDetails, MSFT_WMIERROR_MESSAGE, &value, MI_STRING, 0);
            }
            
            DSC_free(message);
        }
    }
    return r;
}


MI_Result AppendWMIError1ParamID(
                        _Inout_ MI_Instance *cimErrorDetails,
                        _In_ MI_Uint32 errorStringId
                        )
{
    MI_Result r = MI_RESULT_OK;
    Intlstr intlstr = Intlstr_Null;
    GetResourceString(errorStringId, &intlstr);
    if( intlstr.str )
    {
        r = AppendWMIError1Param(cimErrorDetails, MI_T("%s %s"), intlstr.str);
        Intlstr_Free(intlstr);
    }

    return r;
}

_Always_(_Ret_range_(==, result))
MI_Result GetCimMIError(MI_Result result , 
                        _Outptr_result_maybenull_ MI_Instance **cimErrorDetails,
                        _In_ MI_Uint32 errorStringId )
{
    Intlstr intlstr = Intlstr_Null;
    GetResourceString(errorStringId, &intlstr);
    
    MI_Utilities_CimErrorFromErrorCode( (MI_Uint32)result, MI_RESULT_TYPE_MI, intlstr.str, cimErrorDetails);
    DSC_EventWriteCIMError(intlstr.str,(MI_Uint32)result);
    if( intlstr.str)
        Intlstr_Free(intlstr);
    
    return result;
}

MI_Result GetCimWin32Error(MI_Uint32 result , 
                        _Outptr_result_maybenull_ MI_Instance **cimErrorDetails,
                        _In_ MI_Uint32 errorStringId )
{
    Intlstr intlstr = Intlstr_Null;
    GetResourceString(errorStringId, &intlstr);
    
    MI_Utilities_CimErrorFromErrorCode( (MI_Uint32)result, MI_RESULT_TYPE_WIN32, intlstr.str, cimErrorDetails);
    DSC_EventWriteCIMError(intlstr.str,(MI_Uint32)result);
    if( intlstr.str)
        Intlstr_Free(intlstr);
#if defined(_MSC_VER)    
    return MIResultFromHRESULT(HRESULT_FROM_WIN32(result));
#else
    return MI_RESULT_FAILED;
#endif
}

_Always_(_Ret_range_(==, result))
MI_Result GetCimMIError1Param(MI_Result result , 
                        _Outptr_result_maybenull_ MI_Instance **cimErrorDetails,
                        _In_ MI_Uint32 errorStringId,
                        _In_z_ const MI_Char * param1)
{
    BOOL errorInitialized = FALSE;
    Intlstr resIntlstr = Intlstr_Null;
    
    GetResourceString1Param(errorStringId, param1, &resIntlstr);
    if( resIntlstr.str )
    {                  
        MI_Utilities_CimErrorFromErrorCode((MI_Uint32)result, MI_RESULT_TYPE_MI, resIntlstr.str, cimErrorDetails);
        DSC_EventWriteCIMError(resIntlstr.str,(MI_Uint32)result);
        errorInitialized = TRUE;
        Intlstr_Free(resIntlstr);
    }     
    if(!errorInitialized)
    {
        MI_Utilities_CimErrorFromErrorCode((MI_Uint32)result, MI_RESULT_TYPE_MI, NULL, cimErrorDetails);
        
    }
    return result;
}

_Always_(_Ret_range_(==, result))
MI_Result GetCimMIError2Params(MI_Result result , 
                        _Outptr_result_maybenull_ MI_Instance **cimErrorDetails,
                        _In_ MI_Uint32 errorStringId,
                        _In_z_ const MI_Char * param1,
                        _In_z_ const MI_Char * param2
                        )
{
    BOOL errorInitialized = FALSE;
    Intlstr resIntlstr = Intlstr_Null;
    
    GetResourceString2Param(errorStringId, param1, param2, &resIntlstr);
    if( resIntlstr.str )
    {                  
        MI_Utilities_CimErrorFromErrorCode((MI_Uint32)result, MI_RESULT_TYPE_MI, resIntlstr.str, cimErrorDetails);
        DSC_EventWriteCIMError(resIntlstr.str,(MI_Uint32)result);
        errorInitialized = TRUE;
        Intlstr_Free(resIntlstr);
    }     
    if(!errorInitialized)
    {
        MI_Utilities_CimErrorFromErrorCode((MI_Uint32)result, MI_RESULT_TYPE_MI, NULL, cimErrorDetails);
        
    }
    return result;

}

_Always_(_Ret_range_(==, result))
MI_Result GetCimMIError3Params(MI_Result result , 
                        _Outptr_result_maybenull_ MI_Instance **cimErrorDetails,
                        _In_ MI_Uint32 errorStringId,
                        _In_z_ const MI_Char * param1,
                        _In_z_ const MI_Char * param2,
                        _In_z_ const MI_Char * param3
                        )
{
    BOOL errorInitialized = FALSE;
    Intlstr resIntlstr = Intlstr_Null;
    
    GetResourceString3Param(errorStringId, param1, param2, param3, &resIntlstr);
    if( resIntlstr.str )
    {                  
        MI_Utilities_CimErrorFromErrorCode((MI_Uint32)result, MI_RESULT_TYPE_MI, resIntlstr.str, cimErrorDetails);
        DSC_EventWriteCIMError(resIntlstr.str,(MI_Uint32)result);
        errorInitialized = TRUE;
        Intlstr_Free(resIntlstr);
    }     
    if(!errorInitialized)
    {
        MI_Utilities_CimErrorFromErrorCode((MI_Uint32)result, MI_RESULT_TYPE_MI, NULL, cimErrorDetails);
        
    }
    return result;

}

_Always_(_Ret_range_(==, result))
MI_Result GetCimMIError4Params(MI_Result result , 
                        _Outptr_result_maybenull_ MI_Instance **cimErrorDetails,
                        _In_ MI_Uint32 errorStringId,
                        _In_z_ const MI_Char * param1,
                        _In_z_ const MI_Char * param2,
                        _In_z_ const MI_Char * param3,
                        _In_z_ const MI_Char * param4
                        )
{
    BOOL errorInitialized = FALSE;
    Intlstr resIntlstr = Intlstr_Null;
    
    GetResourceString4Param(errorStringId, param1, param2, param3, param4, &resIntlstr);
    if( resIntlstr.str )
    {                  
        MI_Utilities_CimErrorFromErrorCode((MI_Uint32)result, MI_RESULT_TYPE_MI, resIntlstr.str, cimErrorDetails);
        DSC_EventWriteCIMError(resIntlstr.str,(MI_Uint32)result);
        errorInitialized = TRUE;
        Intlstr_Free(resIntlstr);
    }     
    if(!errorInitialized)
    {
        MI_Utilities_CimErrorFromErrorCode((MI_Uint32)result, MI_RESULT_TYPE_MI, NULL, cimErrorDetails);
        
    }
    return result;

}


MI_Result AppendWMIErrorWithResourceID(
                        _Inout_ MI_Instance *cimErrorDetails,
                        _In_z_ const MI_Char * resourceId
                        )
{
    MI_Result r = MI_RESULT_OK;
    Intlstr intlstr = Intlstr_Null;
    MI_Value value;

    r = DSC_MI_Instance_GetElement(cimErrorDetails, MSFT_WMIERROR_MESSAGE, &value, NULL, NULL, NULL);
    if( r == MI_RESULT_OK )
    {
        GetResourceString2Param(ID_CA_MOVETODESIREDSTATE_FAILED_APPEND_RESOURCEID, value.string, resourceId, &intlstr);
        if( intlstr.str)
        {
            value.string = (MI_Char*)intlstr.str;
            r = MI_Instance_SetElement(cimErrorDetails, MSFT_WMIERROR_MESSAGE, &value, MI_STRING, 0);
            Intlstr_Free(intlstr);
        }
    }
    return r;
}

MI_Result ResolvePath(_Outptr_opt_result_maybenull_z_ MI_Char **envResolvedPath, 
                      _Outptr_opt_result_maybenull_z_ MI_Char **searchPath, 
                      _In_z_ const MI_Char *envPath,
                      _In_z_ const MI_Char *searchPattern,
                      _Outptr_result_maybenull_ MI_Instance **extendedError)
{
    MI_Uint32 dwReturnSizeInitial = 0, dwReturnSize = (MI_Uint32) (Tcslen(envPath)+1);
    int result = 0;
    const MI_Char *pathToUse = envPath;

    if (extendedError == NULL)
    {        
        return MI_RESULT_INVALID_PARAMETER; 
    }
    *extendedError = NULL;  // Explicitly set *extendedError to NULL as _Outptr_ requires setting this at least once.   

    if( searchPath)
    {
        *searchPath = NULL;
    }

    if( envResolvedPath != NULL )
    {
#if defined(_MSC_VER)        
        dwReturnSizeInitial = ExpandEnvironmentStrings(envPath, NULL, 0);         
#else
        dwReturnSizeInitial = Tcslen(envPath) + 1;
#endif

        *envResolvedPath = (MI_Char*)DSC_malloc(dwReturnSizeInitial* sizeof(MI_Char), NitsHere());
        if( *envResolvedPath == NULL)
        {
            return GetCimMIError(MI_RESULT_SERVER_LIMITS_EXCEEDED, extendedError, ID_LCMHELPER_MEMORY_ERROR);
        }

#if defined(_MSC_VER)
        dwReturnSize = ExpandEnvironmentStrings(envPath, *envResolvedPath, dwReturnSizeInitial);
        if( dwReturnSize == 0 || (dwReturnSize >  dwReturnSizeInitial ) || NitsShouldFault(NitsHere(), NitsAutomatic))
        {
            //memory error
            DSC_free(*envResolvedPath);
            *envResolvedPath = NULL;
            return GetCimMIError(MI_RESULT_SERVER_LIMITS_EXCEEDED, extendedError, ID_LCMHELPER_EXPANDENV_FAILED);
        }
#else
        memcpy(*envResolvedPath, envPath, dwReturnSizeInitial * sizeof(MI_Char));
#endif
        pathToUse = *envResolvedPath;
    }

    if( searchPath != NULL)
    {

        dwReturnSize += (MI_Uint32) (Tcslen(searchPattern) + 1);// %s\\%s

        /* Create Search Path*/
        *searchPath = (MI_Char*)DSC_malloc(dwReturnSize* sizeof(MI_Char), NitsHere()); // %s\\%s
        if( *searchPath == NULL)
        {
            if( envResolvedPath != NULL )
            {
                DSC_free(*envResolvedPath);
                *envResolvedPath = NULL;
            }
            return GetCimMIError(MI_RESULT_SERVER_LIMITS_EXCEEDED, extendedError, ID_LCMHELPER_MEMORY_ERROR);
        }    
#if defined(_MSC_VER)        
        result = Stprintf(*searchPath, dwReturnSize, MI_T("%T\\%T"), pathToUse, searchPattern);
#else
        result = Stprintf(*searchPath, dwReturnSize, MI_T("%T/%T"), pathToUse, searchPattern);
#endif

        if( result <= 0 || NitsShouldFault(NitsHere(), NitsAutomatic))
        {
            if( envResolvedPath != NULL )
            {
                DSC_free(*envResolvedPath);
                *envResolvedPath = NULL;
            }

            DSC_free(*searchPath);
            return GetCimMIError(MI_RESULT_FAILED, extendedError, ID_LCMHELPER_PRINTF_ERROR);
        }    
    }
    return MI_RESULT_OK;
}


void SetJobId()
{
    MI_Char *palUuid;
    if(g_ConfigurationDetails.hasSetDetail==MI_TRUE)
        return; //Which means details were set before.

    palUuid = Generate_UUID(  NitsMakeCallSite(-3, NULL, NULL, 0) );
    if(palUuid == NULL)
    {
        return;
    }
    memcpy(g_ConfigurationDetails.jobGuidString, palUuid, JOB_UUID_LENGTH);
    PAL_Free(palUuid);
    g_ConfigurationDetails.hasSetDetail=MI_TRUE;
}
void ResetJobId()
{
    g_ConfigurationDetails.hasSetDetail=MI_FALSE;
}

void CleanUpClassCache(_Inout_ MI_ClassA *miClassArray)
{
    MI_Uint32 xCount = 0;
    if( miClassArray == NULL || miClassArray->size == 0 )
    {
        return;
    }
    for(xCount = 0; xCount < miClassArray->size; xCount++)
    {
        MI_Class_Delete(miClassArray->data[xCount]);
    }
    DSC_free(miClassArray->data);
}

void CleanUpInstanceCache(_Inout_ MI_InstanceA *instanceArray)
{
    MI_Uint32 xCount = 0;
    if( instanceArray == NULL || instanceArray->size == 0 )
    {
        return;
    }
    for(xCount = 0; xCount < instanceArray->size; xCount++)
    {
        MI_Instance_Delete(instanceArray->data[xCount]);
    }
    DSC_free(instanceArray->data);
}

void InitLocTable()
{
    // Sort the table based on locId.
    MI_Uint32 xCount, yCount;
    Loc_Mapping tempMapping;
    for(xCount = 0; xCount < g_LocMappingTableSize; xCount++)
    {
        for( yCount = xCount+1; yCount < g_LocMappingTableSize; yCount++)
        {
            if( g_LocMappingTable[xCount].locId > g_LocMappingTable[yCount].locId )
            {
                //swap
                memcpy(&tempMapping, &g_LocMappingTable[xCount], sizeof(Loc_Mapping));
                memcpy(&g_LocMappingTable[xCount], &g_LocMappingTable[yCount], sizeof(Loc_Mapping));
                memcpy(&g_LocMappingTable[yCount], &tempMapping,sizeof(Loc_Mapping));
            }
        }
    }
}

 int Get_LocMappingIndex(_In_ MI_Uint32 errorStringId)
{
    MI_Uint32 high = g_LocMappingTableSize-1;
    MI_Uint32 low = 0;
    while( low<=high)
    {
        MI_Uint32 mid = (low+high)/2;
        if( errorStringId < g_LocMappingTable[mid].locId )
        {
            high = mid - 1;
        }
        else if( errorStringId > g_LocMappingTable[mid].locId )
        {
            low = mid+1;
        }        
        else
        {
            return mid;
        }
        
    }
    return -1;
}


void GetResourceString( _In_ MI_Uint32 errorStringId, _Inout_ Intlstr *resStr)
{
   int index = Get_LocMappingIndex(errorStringId);  
   if( index >= 0 )
   {
       *resStr = g_LocMappingTable[index].LocFunctionZeroArgs();
   }      
}


void GetResourceString1Param( _In_ MI_Uint32 errorStringId, _In_z_ const MI_Char * param1, _Inout_ Intlstr *resStr)
{
   int index = Get_LocMappingIndex(errorStringId);    
   if( index >= 0 )
   {
       *resStr = g_LocMappingTable[index].LocFunctionOneArgs((MI_Char*)param1);
   }  
}

void GetResourceString2Param( _In_ MI_Uint32 errorStringId, _In_z_ const MI_Char * param1,
                               _In_z_ const MI_Char * param2,_Inout_ Intlstr *resStr)
{
   int index = Get_LocMappingIndex(errorStringId);    
   if( index >= 0 )
   {
       *resStr = g_LocMappingTable[index].LocFunctionTwoArgs((MI_Char*)param1, (MI_Char*)param2);
   }  
}
 void GetResourceString3Param( _In_ MI_Uint32 errorStringId, _In_z_ const MI_Char * param1,
               _In_z_ const MI_Char * param2, _In_z_ const MI_Char * param3,_Inout_ Intlstr *resStr)
{
   int index = Get_LocMappingIndex(errorStringId);    
   if( index >= 0 )
   {
       *resStr = g_LocMappingTable[index].LocFunctionThreeArgs((MI_Char*)param1, (MI_Char*)param2, (MI_Char*)param3);
   }  
}

void GetResourceString4Param( _In_ MI_Uint32 errorStringId, _In_z_ const MI_Char * param1, 
               _In_z_ const MI_Char * param2, _In_z_ const MI_Char * param3,  _In_z_ const MI_Char * param4, _Inout_ Intlstr *resStr)
{
   int index = Get_LocMappingIndex(errorStringId);    
   if( index >= 0 )
   {
       *resStr = g_LocMappingTable[index].LocFunctionFourArgs((MI_Char*)param1, (MI_Char*)param2, (MI_Char*)param3, (MI_Char*)param4);
   }  
}  

void CleanUpDeserializerClassCache(_Inout_ MI_ClassA *miClassArray)
{
    if( miClassArray == NULL || miClassArray->size == 0 )
    {
        return;
    }

    MI_Deserializer_ReleaseClassArray(miClassArray);
}

void CleanUpDeserializerInstanceCache(_Inout_ MI_InstanceA *instanceArray)
{
    if( instanceArray == NULL || instanceArray->size == 0 )
    {
        return;
    }

    MI_Deserializer_ReleaseInstanceArray(instanceArray);
}

/*caller will cleanup inputInstanceArray and outputInstanceArray*/
MI_Result UpdateInstanceArray(_In_ MI_InstanceA *inputInstanceArray,
                         _Inout_ MI_InstanceA *outputInstanceArray,
                        _Outptr_result_maybenull_ MI_Instance **extendedError,
                        _In_ MI_Boolean bInputUsingSerializedAPI)
{
    MI_Uint32 xCount = 0;
    MI_Uint32 newSize = inputInstanceArray->size + outputInstanceArray->size;
    MI_Instance **tempOutput = NULL;

    if (extendedError == NULL)
    {        
        return MI_RESULT_INVALID_PARAMETER; 
    }
    *extendedError = NULL;  // Explicitly set *extendedError to NULL as _Outptr_ requires setting this at least once.   
    
    if( inputInstanceArray->size == 0 )
    {
        return MI_RESULT_OK;
    }
    tempOutput = (MI_Instance **)DSC_malloc( newSize * sizeof(MI_Instance*), NitsHere());
    if(tempOutput == NULL)
    {
        return GetCimMIError(MI_RESULT_SERVER_LIMITS_EXCEEDED, extendedError, ID_ENGINEHELPER_MEMORY_ERROR);
    }
    /*Copy current list to bigger list*/
    for( xCount = 0 ; xCount < outputInstanceArray->size; xCount++)
    {
        tempOutput[xCount] = outputInstanceArray->data[xCount];
    }
    /*Copy new list to bigger list*/
    for( xCount = outputInstanceArray->size ; xCount < newSize; xCount++)
    {
        tempOutput[xCount] = inputInstanceArray->data[xCount - outputInstanceArray->size];
        inputInstanceArray->data[xCount - outputInstanceArray->size] = NULL;
    }

    /*Free memory and update pointers*/
    if( bInputUsingSerializedAPI)
        MI_Deserializer_ReleaseInstanceArray(inputInstanceArray);
    else
        DSC_free(inputInstanceArray->data);

    DSC_free(outputInstanceArray->data);
    outputInstanceArray->data = tempOutput;
    outputInstanceArray->size = newSize;
    return MI_RESULT_OK;
}

void CleanUpGetCache(_Inout_ MI_InstanceA *instanceArray)
{
    if( instanceArray == NULL )
    {
        return;
    }

    MI_Deserializer_ReleaseInstanceArray(instanceArray);
}



/*caller will cleanup inputClassArray and outputClassArray*/
MI_Result UpdateClassArray(_In_ MI_ClassA *inputClassArray,
                           _Inout_ MI_ClassA *outputClassArray,
                           _Outptr_result_maybenull_ MI_Instance **extendedError,
                           _In_ MI_Boolean bInputUsingSerializedAPI)
{
    MI_Uint32 xCount = 0;
    MI_Uint32 newSize = inputClassArray->size + outputClassArray->size;
    MI_Class **tempOutput = NULL;
    
    if (extendedError == NULL)
    {        
        return MI_RESULT_INVALID_PARAMETER; 
    }
    *extendedError = NULL;  // Explicitly set *extendedError to NULL as _Outptr_ requires setting this at least once.   

    if( inputClassArray->size == 0 )
    {
        return MI_RESULT_OK;
    }
    tempOutput = (MI_Class **)DSC_malloc( newSize * sizeof(MI_Class*), NitsHere());
    if( tempOutput == NULL)
    {
        return GetCimMIError(MI_RESULT_SERVER_LIMITS_EXCEEDED, extendedError, ID_ENGINEHELPER_MEMORY_ERROR);
    }
    /*Copy current list to bigger list*/
    for( xCount = 0 ; xCount < outputClassArray->size; xCount++)
    {
        tempOutput[xCount] = outputClassArray->data[xCount];
    }
    /*Copy new list to bigger list*/
    for( xCount = outputClassArray->size ; xCount < newSize; xCount++)
    {
        tempOutput[xCount] = inputClassArray->data[xCount - outputClassArray->size];
        inputClassArray->data[xCount - outputClassArray->size] = NULL;
    }

    /*Update pointers*/
    if( bInputUsingSerializedAPI)
        MI_Deserializer_ReleaseClassArray(inputClassArray);
    else
        DSC_free(inputClassArray->data);

    DSC_free(outputClassArray->data);
    outputClassArray->data = tempOutput;
    outputClassArray->size = newSize;
    return MI_RESULT_OK;

}

const MI_Char * GetDownloadManagerName( _In_ MI_Instance *inst)
{
    MI_Result r = MI_RESULT_OK;
    MI_Value value;
    r = MI_Instance_GetElement(inst, MSFT_DSCMetaConfiguration_DownloadManagerName, &value, NULL, NULL, NULL);
    if( r != MI_RESULT_OK)
    {
        return NULL;
    }
    return (const MI_Char*)value.string;
}

const MI_Char * GetErrorDetail( _In_ MI_Instance *inst)
{
    MI_Result r = MI_RESULT_OK;
    MI_Value value;
    r = MI_Instance_GetElement(inst, MI_T("Message"), &value, NULL, NULL, NULL);
    if( r != MI_RESULT_OK)
    {
        return NULL;
    }
    return (const MI_Char*)value.string;
}


/* caller will need to release the memory  for content buffer */
MI_Result ReadFileContent(_In_z_ const MI_Char *pFileName,
                          _Outptr_result_buffer_maybenull_(*pBufferSize) MI_Uint8 ** pBuffer, 
                          _Out_ MI_Uint32 * pBufferSize,
                          _Outptr_result_maybenull_ MI_Instance **cimErrorDetails)
{
    FILE *fp;
    size_t result;
    unsigned long fileLen;
    size_t nSizeWritten;

    *pBuffer = 0;
    *pBufferSize = 0;
    fp = File_OpenT(pFileName, MI_T("rb"));
    if( fp == NULL)
    {
        return GetCimMIError(MI_RESULT_FAILED, cimErrorDetails, ID_ENGINEHELPER_OPENFILE_ERROR);
    }
    
    // Get File size
    result = fseek(fp, 0, SEEK_END);
    if(result)
    {
        File_Close(fp);
        return GetCimMIError(MI_RESULT_SERVER_LIMITS_EXCEEDED, cimErrorDetails, ID_ENGINEHELPER_FILESIZE_ERROR);
    }
    fileLen = ftell(fp);
    if(fileLen > MAX_MOFSIZE )
    {
        File_Close(fp);
        return GetCimMIError(MI_RESULT_SERVER_LIMITS_EXCEEDED, cimErrorDetails, ID_ENGINEHELPER_FILESIZE_ERROR);
    }    
    result = fseek(fp, 0, SEEK_SET);
    if(result)
    {
        File_Close(fp);
        return GetCimMIError(MI_RESULT_SERVER_LIMITS_EXCEEDED, cimErrorDetails, ID_ENGINEHELPER_FILESIZE_ERROR);
    }

    *pBuffer = (MI_Uint8*)DSC_malloc(fileLen, NitsHere());
    if(*pBuffer == NULL)
    {
        File_Close(fp);
        return GetCimMIError(MI_RESULT_SERVER_LIMITS_EXCEEDED, cimErrorDetails, ID_ENGINEHELPER_FILESIZE_ERROR);
    }

    // Read the contents

#if defined(_MSC_VER)
            nSizeWritten = (long)fread(*pBuffer, 1, fileLen, fp);
#else
            nSizeWritten = fread(*pBuffer, 1, fileLen, fp);
#endif

    File_Close(fp);
    if( nSizeWritten != fileLen)
    {
        DSC_free(*pBuffer);
        *pBuffer = NULL;
        return GetCimMIError(MI_RESULT_SERVER_LIMITS_EXCEEDED, cimErrorDetails, ID_ENGINEHELPER_READFILE_ERROR);        
    }
    *pBufferSize = (MI_Uint32)fileLen;
    return MI_RESULT_OK;
}



void CleanupTempDirectory(_In_z_ MI_Char *mofFileName)
{
    /*It is ok if we fail to cleanup temp directory.*/
    MI_Char *lastOccurancePointer = Tcsrchr(mofFileName, MI_T('\\'));
    if( lastOccurancePointer != NULL )
    {
        size_t lastOccuranceIndex = 0; 
        lastOccuranceIndex = lastOccurancePointer - mofFileName;
        if( Tcslen(mofFileName) <= lastOccuranceIndex+1 )
        {
            mofFileName[lastOccuranceIndex] =  MI_T('\0');
            RecursivelyDeleteDirectory(mofFileName);
        }
    }
}

MI_Boolean IsConfirmUsed(_In_opt_ MI_Context* context)
{
    MI_Result result=MI_RESULT_OK;
    MI_Type type;
    MI_Value value;
    MI_Boolean confirmUsedFlag=MI_FALSE;
    //In the case of CIM cmdlets, the option gets set at a particular index whenever whatif is called.
    if(context)
    {
        result= MI_Context_GetCustomOptionAt(context,DSC_INDEX_OPTION_CONFIRM,NULL,&type,&value);
    
        if(result==MI_RESULT_OK && type==MI_SINT32 && value.sint32 == DSC_CONFIRMOPTION_SET_VALUE)
        {
            confirmUsedFlag=MI_TRUE;
        }
    }
    return confirmUsedFlag;

}

void RecursivelyDeleteDirectory(_In_z_ MI_Char *directoryPath)
{
    MI_Char pathTempVar[MAX_PATH];
    Internal_Dir *dirHandle = NULL;
    Internal_DirEnt *dirEntry = NULL;
    dirHandle = Internal_Dir_Open(directoryPath, NitsMakeCallSite(-3, NULL, NULL, 0) );
    if( dirHandle != NULL )
    {
        dirEntry =  Internal_Dir_Read(dirHandle, NULL);
        
        while (dirEntry != NULL )
        {
#if defined(_MSC_VER)            
            if(Stprintf(pathTempVar, MAX_PATH, MI_T("%T\\%T"), directoryPath, dirEntry->name) >0 )
#else
            if(Stprintf(pathTempVar, MAX_PATH, MI_T("%T/%T"), directoryPath, dirEntry->name) >0 )
#endif          
            {
                if(Tcscasecmp(MI_T(".."), dirEntry->name) == 0 ||
                   Tcscasecmp(MI_T("."), dirEntry->name)==0 )
                {
                }
                else if(dirEntry->isDir)
                {
                    RecursivelyDeleteDirectory(pathTempVar);
                }
                else
                {
                    File_RemoveT(pathTempVar);
                }                    
            }
            dirEntry =  Internal_Dir_Read(dirHandle, NULL); //Next
        }
        
        Internal_Dir_Close( dirHandle);
    }
    Directory_Remove(directoryPath);
}

void SQMLogResourceCountData(_In_z_ const MI_Char* providerName,_In_ MI_Uint32 resourceCount)
{


/* Windows feature*/
#if defined(_MSC_VER)
  
    //Increments the value present for the provider index, with a +valueIncrement. (if there are valueincrement number of resources processed for that resource)
    SQM_STREAM_ENTRY_EX resourceCountDetail[SQM_PARAMETER_SIZE] = {0};
    HSESSION sqmSession;
    GUID sessionGuid;
    if (FAILED(CoCreateGuid(&sessionGuid)))
    {
        return;
    }
    //Start a session for the DSC Datapoint called ResourceCount - Usea  known session GUID
    sqmSession=WinSqmStartSession(&sessionGuid, SQM_MACHINEGLOBAL_SESSIONID, 0);
    if (sqmSession == INVALID_HANDLE_VALUE)
    {
        return;
    }
    // Enter the provider name entry in the stream  
    WinSqmCreateStringStreamEntryEx(&resourceCountDetail[SQM_INDEX_PROVIDERNAME],providerName);

    //Enter the resource count entry in stream
    WinSqmCreateDWORDStreamEntryEx(&resourceCountDetail[SQM_INDEX_RESOURCECOUNT],resourceCount);

    // Record the complete row.
    WinSqmAddToStreamEx(NULL,
                        SQM_RESOURCECOUNT_DATAPOINTID,
                        SQM_PARAMETER_SIZE,
                        resourceCountDetail,
                        0);

    //If the value hasn't been set before, increment sets the value to valueIncrement.
    WinSqmEndSession(sqmSession);

#endif
}

