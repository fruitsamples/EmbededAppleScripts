/*	File:		ScriptSupport.c		Description:	General routines for handling the details involved in calling AppleScript	script handlers from C.  These routines are not tied to any particular script,	so they have been spared out from the rest in this general utility file.	Author:		JM	Copyright: 	Copyright (c) 2003 Apple Computer, Inc. All rights reserved.		Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.				("Apple") in consideration of your agreement to the following terms, and your				use, installation, modification or redistribution of this Apple software				constitutes acceptance of these terms.  If you do not agree with these terms,				please do not use, install, modify or redistribute this Apple software.				In consideration of your agreement to abide by the following terms, and subject				to these terms, Apple grants you a personal, non-exclusive license, under Apple�s				copyrights in this original Apple software (the "Apple Software"), to use,				reproduce, modify and redistribute the Apple Software, with or without				modifications, in source and/or binary forms; provided that if you redistribute				the Apple Software in its entirety and without modifications, you must retain				this notice and the following text and disclaimers in all such redistributions of				the Apple Software.  Neither the name, trademarks, service marks or logos of				Apple Computer, Inc. may be used to endorse or promote products derived from the				Apple Software without specific prior written permission from Apple.  Except as				expressly stated in this notice, no other rights or licenses, express or implied,				are granted by Apple herein, including but not limited to any patent rights that				may be infringed by your derivative works or by other works in which the Apple				Software may be incorporated.				The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO				WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED				WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR				PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN				COMBINATION WITH YOUR PRODUCTS.				IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR				CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE				GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)				ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION				OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT				(INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN				ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.					Change History (most recent first):        Fri, Aug 29, 2000 -- created*/#include "ScriptSupport.h"#include <stdio.h>OSStatus LoadCompiledScript(CFStringRef scriptRName, LoadedScriptInfoPtr *scriptInfo) {    CFBundleRef mainbundle;    CFURLRef resourceURL;    Boolean result;	OSStatus err;	char filepath[8000];	FILE* f;	long length, actread;	char* buffer;    ComponentInstance theComponent;	AEDesc scriptData;	OSAID contextID;	LoadedScriptInfoPtr localscriptinfo;	Boolean contextAllocated;			/* set up locals to a known explicit state*/	result = false;	err = noErr;	resourceURL = NULL;	f = NULL;	buffer = NULL;	theComponent = NULL;	contextID = kOSANullScript;	localscriptinfo = NULL;	contextAllocated = false;			/* get the main bundle */	mainbundle = CFBundleGetMainBundle();	if ( NULL == mainbundle ) {		err = paramErr;	}			/* find our resource's URL */	if ( noErr == err ) {		resourceURL = CFBundleCopyResourceURL(mainbundle, scriptRName, CFSTR("scpt"), NULL);		if ( NULL == resourceURL ) {			err = fnfErr;		}	}			/* convert the resource URL into a file system path */	if ( ! CFURLGetFileSystemRepresentation(resourceURL, false, filepath, sizeof(filepath)) ) {		err = fnfErr;	}			/* we're done with the url now */	if ( NULL != resourceURL ) {		CFRelease(resourceURL);	}			/* open the file */	if ( noErr == err) {		f = fopen(filepath, "r");		if ( NULL == f ) {			err = fnfErr;		} else {			fseek(f, 0L, SEEK_END);			length = ftell(f);			fseek(f, 0L, SEEK_SET);		}	}			/* allocate a buffer for reading the file's contents */	if ( noErr == err ) {		buffer = (char*) malloc(length);		if ( NULL == buffer ) {			err = memFullErr;		}	}			/* read the contents */	if ( noErr == err ) {		actread = fread(buffer, 1, length, f);		if ( actread != length ) {			err = paramErr;		}	}			/* we're done with the file now */	if ( NULL != f ) {		fclose(f);		f = NULL;	}			/* convert the contents into an AEDesc */	if ( noErr == err ) {		err = AECreateDesc(typeOSAGenericStorage, buffer, length, &scriptData);	}			/* we're done with the data buffer now */	if ( NULL != buffer ) {		free(buffer);		buffer = NULL;	}	        /* open the scripting component */	if ( noErr == err ) {		theComponent = OpenDefaultComponent(kOSAComponentType, typeAppleScript);		if (theComponent == NULL) {			err = paramErr;		}	}	        /* load the script into a new context */	if ( noErr == err ) {		err = OSALoad(theComponent, &scriptData, kOSAModeNull, &contextID);		if ( noErr == err ) {			contextAllocated = true;		}	}		/* we're done with the descriptor now */	if ( NULL != buffer ) {		AEDisposeDesc(&scriptData);	}        /* if everything went as planned, create a record to pass back to the caller */	if ( noErr == err ) {		localscriptinfo = (LoadedScriptInfoPtr) malloc(sizeof(LoadedScriptInfo));		if ( NULL == localscriptinfo) {			err = memFullErr;		}	}			/* save our fields, store result parameter */	if ( noErr == err ) {		localscriptinfo->fComponent = theComponent;		localscriptinfo->fContextID = contextID;		*scriptInfo = localscriptinfo;	} else  {			/* clean up on error  */		if ( contextAllocated ) {			OSADispose(theComponent, contextID);		}		if ( NULL != theComponent ) {			CloseComponent(theComponent);		}	}			/* return status result  */    return err;}void UnloadCompiledScript( LoadedScriptInfoPtr scriptInfo ) {	if ( NULL != scriptInfo ) {			/* release the compiled script context */		OSADispose(scriptInfo->fComponent, scriptInfo->fContextID);			/* close our component */		CloseComponent(scriptInfo->fComponent);			/* free our persistent state */		free(scriptInfo);	}}	/* AEBuildErrorToAEDesc converts an AEBuildError error descriptor into	a unicode text AEDesc.  If an error occurs, then the descriptor is	initialized to a typeNull desc. This is a utility routine for finding	problems in your AEBuild strings. */void AEBuildErrorToAEDesc(AEBuildError *error, AEDesc *errorTextDesc) {	CFStringRef errDesc;	CFStringRef errorString;	OSStatus err;	switch ( error->fError ) {		case aeBuildSyntaxNoErr:			errDesc = CFSTR("(No error)");			break;		case aeBuildSyntaxBadToken:			errDesc = CFSTR("Illegal character");			break;		case aeBuildSyntaxBadEOF:			errDesc = CFSTR("Unexpected end of format string");			break;		case aeBuildSyntaxNoEOF:			errDesc = CFSTR("Unexpected extra stuff past end");			break;		case aeBuildSyntaxBadNegative:			errDesc = CFSTR("\"-\" not followed by digits");			break;		case aeBuildSyntaxMissingQuote:			errDesc = CFSTR("Missing close \"'\"");			break;		case aeBuildSyntaxBadHex:			errDesc = CFSTR("Non-digit in hex string");			break;		case aeBuildSyntaxOddHex:			errDesc = CFSTR("Odd # of hex digits");			break;		case aeBuildSyntaxNoCloseHex:			errDesc = CFSTR("Missing $ or \"�\"");			break;		case aeBuildSyntaxUncoercedHex:			errDesc = CFSTR("Hex string must be coerced to a type");			break;		case aeBuildSyntaxNoCloseString:			errDesc = CFSTR("Missing closing quote");			break;		case aeBuildSyntaxBadDesc:			errDesc = CFSTR("Illegal descriptor");			break;		case aeBuildSyntaxBadData:			errDesc = CFSTR("Bad data value inside (...)");			break;		case aeBuildSyntaxNoCloseParen:			errDesc = CFSTR("Missing \")\" after data value");			break;		case aeBuildSyntaxNoCloseBracket:			errDesc = CFSTR("Expected \",\" or \"]\"");			break;		case aeBuildSyntaxNoCloseBrace:			errDesc = CFSTR("Expected \",\" or \"}\"");			break;		case aeBuildSyntaxNoKey:			errDesc = CFSTR("Missing keyword in record");			break;		case aeBuildSyntaxNoColon:			errDesc = CFSTR("Missing \":\" after keyword in record");			break;		case aeBuildSyntaxCoercedList:			errDesc = CFSTR("Cannot coerce a list");			break;		case aeBuildSyntaxUncoercedDoubleAt:			errDesc = CFSTR("\"@@\" substitution must be coerced");			break;		default:			errDesc = CFSTR("unknown error");			break;	}	errorString = CFStringCreateWithFormat(NULL, NULL, 				CFSTR("Error %d (%s) found at position %d."),				error->fError, errDesc, error->fErrorPos);	if ( NULL == errorString ) {		AECreateDesc(typeNull, NULL, 0, errorTextDesc);	} else {		err = CFStringToAEDesc( errorString, errorTextDesc );		if ( noErr != err ) {			AECreateDesc(typeNull, NULL, 0, errorTextDesc);		}		CFRelease(errorString);	}}OSStatus CallScriptSubroutine(				LoadedScriptInfoPtr scriptInfo, /* returned by LoadCompiledScript */				char* subroutineName, /* the name off the subroutine - must be all lowercase */				AEDescList *subroutineParameters, /* positionally ordered list of parameters */				AEDesc *resultData, /* returned data from the script - can be NULL */				AEDesc *errorMessages /* error messages - can be NULL */				) {    OSStatus err;    OSAID resultID;    ProcessSerialNumber PSN = {0, kCurrentProcess};    AppleEvent theAEvent;	AEBuildError buildErr;			/* verify parameters */	if ( NULL == scriptInfo	|| NULL == subroutineName	|| NULL == subroutineParameters )		return paramErr;			/* build the subroutine call event */    err = AEBuildAppleEvent(        typeAppleScript, kASSubroutineEvent,        typeProcessSerialNumber, (Ptr) &PSN, sizeof(PSN),        kAutoGenerateReturnID, kAnyTransactionID,        &theAEvent, &buildErr,        "'----':@,'snam':TEXT(@)",		subroutineParameters, subroutineName);	if ( noErr != err ) {			/* on error, try to fill out the message */		if ( NULL != errorMessages ) {			AEBuildErrorToAEDesc(&buildErr, errorMessages);		}	} else {				/* run the script */		err = OSAExecuteEvent(					scriptInfo->fComponent,					&theAEvent,					scriptInfo->fContextID,					kOSAModeNull,					&resultID);							switch ( err ) {						/* if no error occured, then collect the result data */			case noErr:				if ( NULL != resultData ) {					err = OSACoerceToDesc(scriptInfo->fComponent, resultID,								typeWildCard, kOSAModeNull, resultData);					if ( ( noErr != err ) && ( NULL != errorMessages ) ) {						AECreateDesc(typeNull, NULL, 0, errorMessages);					}				}				OSADispose(scriptInfo->fComponent, resultID);				break;							/* try to get the resulting error text */			case errOSAScriptError:				if ( NULL != errorMessages ) {					err = OSAScriptError(scriptInfo->fComponent, kOSAErrorMessage,								typeChar, errorMessages);					if ( noErr != err ) {						AECreateDesc(typeNull, NULL, 0, errorMessages);					}					err = errOSAScriptError;				}				break;							/* no error text for other errors */			default:				if ( NULL != errorMessages ) {					AECreateDesc(typeNull, NULL, 0, errorMessages);				}				break;		}		AEDisposeDesc(&theAEvent);	}        /* return our result */    return err;}OSStatus AEDescToAlias( AEDesc *aliasDesc, AliasHandle *theAlias) {	OSStatus err;	AEDesc theAliasDesc;	err = AECoerceDesc( aliasDesc, typeAlias, &theAliasDesc);	if ( noErr == err ) {		Size bytecount;		AliasHandle localAlias;				bytecount = AEGetDescDataSize(&theAliasDesc);		localAlias = (AliasHandle) NewHandle(bytecount);		if ( NULL == localAlias ) {			err = memFullErr;		} else {			HLock((Handle) localAlias);			AEGetDescData(&theAliasDesc, *localAlias, bytecount);			HUnlock((Handle) localAlias);			*theAlias = localAlias;		}		AEDisposeDesc(&theAliasDesc);	}	return err;}OSStatus AliasToAEDesc( AliasHandle theAlias, AEDesc *theAliasDesc  ) {		/* here's a simple one, we'll just use AEBuildDesc to		take care of all the details... See technote 2045 for details		about AEBuildDesc. */	return AEBuildDesc( theAliasDesc, NULL, "alis(@@)", theAlias);}OSStatus AEDescToCFString( AEDesc *stringDescriptor, CFStringRef *theString ) {	OSStatus err;	AEDesc theUnicodeDesc;	err = AECoerceDesc( stringDescriptor, typeUnicodeText, &theUnicodeDesc);	if ( noErr == err ) {		Size bytecount;		UniChar *localChars;		CFStringRef resultString;				bytecount = AEGetDescDataSize(&theUnicodeDesc);		localChars = (UniChar *) malloc(bytecount);		if ( NULL == localChars ) {			err = memFullErr;		} else {			AEGetDescData(&theUnicodeDesc, localChars, bytecount);			resultString = CFStringCreateWithCharacters(NULL, localChars, bytecount / sizeof(UniChar));			if ( resultString == NULL ) {				err = coreFoundationUnknownErr;			} else {				*theString = resultString;			}			free(localChars);		}		AEDisposeDesc(&theUnicodeDesc);	}	return err;}OSStatus CFStringToAEDesc( CFStringRef theString, AEDesc *theUnicodeDesc ) {	OSStatus err;	CFIndex length, bytecount;	UniChar *localChars;	AEDesc localUnicodeDesc;		length = CFStringGetLength(theString);	bytecount = ( length * sizeof(UniChar) );	localChars = (UniChar *) malloc( bytecount );	if ( NULL == localChars ) {		err = memFullErr;	} else {		CFStringGetCharacters(theString, CFRangeMake(0, length), localChars);		err = AECreateDesc(typeUnicodeText, localChars, bytecount, &localUnicodeDesc);		if ( noErr == err ) {			*theUnicodeDesc = localUnicodeDesc;		}		free( localChars );	}	return err;}	/* AEDescToBoolean and BooleanToAEDesc are convenience routines for	converting Boolean values to and from AEDesc records */OSStatus AEDescToBoolean( AEDesc *boolDesc, Boolean *theValue ) {	OSStatus err;	AEDesc theBoolDesc;	err = AECoerceDesc( boolDesc,  typeBoolean, &theBoolDesc);	if ( noErr == err ) {		AEGetDescData(&theBoolDesc, theValue, sizeof(*theValue));		AEDisposeDesc(&theBoolDesc);	}	return err;}OSStatus BooleanToAEDesc( Boolean theValue, AEDesc *theBoolDesc  ) {		/* here's a simple one, we'll just use AEBuildDesc to		take care of all the details... See technote 2045 for details		about AEBuildDesc.*/	return AEBuildDesc( theBoolDesc, NULL, "bool(@)", theValue);}	/* AEDescToLong and LongToAEDesc are convenience routines for	converting integer values to and from AEDesc records */OSStatus AEDescToLong( AEDesc *longDesc, long *theValue ) {	OSStatus err;	AEDesc theLongDesc;	err = AECoerceDesc( longDesc, typeLongInteger, &theLongDesc);	if ( noErr == err ) {		AEGetDescData(&theLongDesc, theValue, sizeof(*theValue));		AEDisposeDesc(&theLongDesc);	}	return err;}OSStatus LongToAEDesc( long theValue, AEDesc *theLongDesc  ) {		/* here's a simple one, we'll just use AEBuildDesc to		take care of all the details... here, we ask it to coerce		the long parameter into a descriptor. See technote 2045 for details		about AEBuildDesc.*/	return AEBuildDesc( theLongDesc, NULL, "long(@)", theValue);}