#ifndef __USERCOPY_HPP__
#define __USERCOPY_HPP__
//#pragma title("usercopy.hpp- class definitions for usercopy")
/*
================================================================================

   (c) Copyright 1995-1998, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

 Program    - usercopy
 Class      - LAN Manager Utilities
 Author     - Christy Boles
 Created    - 09/04/97
 Description- class definitions to allow usercopy to process subsets of accounts.
              The list of accounts will be generated by the GUI, and will consist 
              of a TNodeList of TAcctNodes.  Users will be added from the front of 
              the list, and groups will be added at the end.


 Updates    - 01/30/98 CAB Added strong password generation


================================================================================
*/

#include <lmcons.h>

#include "TNode.hpp"
#include <share.h>              // for _SH_DENYNO
#include "EaLen.hpp"
#include "Common.hpp"
#include "Err.hpp"
#include "UString.hpp"
#include "CommaLog.hpp"
#include "TARNode.hpp"
#include "WorkObj.h"
#include "ProcExts.h"
//#import  "\bin\DBManager.tlb" no_namespace, named_guids
//#import "\bin\McsDctWorkerObjects.tlb"
#import  "DBMgr.tlb" no_namespace, named_guids
#import  "WorkObj.tlb"

#define AR_BUFSIZE        ((size_t)16000)
#define AR_NUM_IN_BUF 5000  

#define F_REPLACE             0x00000001  // replace account info
#define F_GROUP               0x00000002  // copy global groups
#define F_LGROUP              0x00000004  // copy local groups
#define F_USERS               0x00000008  // copy users
#define F_DISABLE_ALL         0x00000010  // disable all accounts
#define F_DISABLE_SPECIAL     0x00000020  // disable Account Ops, Backup Ops, Administrators, Domain Admins
#define F_STRONGPW_ALL        0x00000040  // generate strong passwords for all accounts
#define F_STRONGPW_SPECIAL    0x00000080  // generate strong passwords for Account Ops, Backup Ops, Admins, and Domain Admins
#define F_MACHINE             0x00000100  // copy computer accounts
#define F_REMOVE_OLD_MEMBERS  0x00000200  // remove old members from replaced groups
#define F_DISABLESOURCE       0x00000400  // disable copied user accounts on source domain
#define F_AddToSrcGroupLocal  0x00000800  // indicates that the add-to group is on the target domain
#define F_AddToGroupLocal     0x00001000  // add to group is a local group
#define F_INTERACT            0x00002000  // use command-line parms to initiate interactive gui session
#define F_WARN_FULLNAME       0x00004000  // warn before replacing accounts w/different fullname 
#define F_WARN_COMMENT        0x00008000  // warn before replacing accounts w/different comment
#define F_CopyPasswords       0x00010000  // copy passwords
#define F_RevokeOldRights     0x00020000  // removes old user rights from copied accounts
#define F_AddSidHistory       0x00040000  // Add SID of source account to the SID history of the target account.
#define F_TranslateProfiles   0x00080000  // Translate roaming profiles
#define F_OUS                 0x00100000  // Process the organizational units.
#define F_COMPUTERS           0x00200000  // Process the computer accounts in Acct replication
#define F_COPY_CONT_CONTENT   0x00400000  // Copy the container contents along with the container when copying accounts.
#define F_COPY_MIGRATED_ACCT  0x00800000  // When expanding containers/membership include accounts that have already been migrated.
#define F_MOVE_REPLACED_ACCT  0x01000000  // move a replaces account to the user-specified OU.
    
#define AR_AccountComputer    (0x80000000)
#define AR_AccountComputerPdc (0x40000000)


#define ADMINISTRATORS     1
#define ACCOUNT_OPERATORS  2
#define BACKUP_OPERATORS   3 
#define DOMAIN_ADMINS      4
#define CREATOR_OWNER      5
#define DOMAIN_USERS       6
#define DOMAIN_CONTROLLERS 7
#define DOMAIN_COMPUTERS   8

struct AccountStats
{
   long                      users;
   long                      globals;
   long                      locals;
   long                      computers;
   long                      generic;
};

class TANode:public TNode
{
   BOOL           bMarked;
   PSID           pSid;
   WCHAR          name[LEN_Account];
   
public:
   TANode() { name[0] = 0; bMarked = FALSE; pSid = NULL;}
   TANode(WCHAR const * n)
   {
      safecopy(name,n);
      bMarked = FALSE;
      pSid = NULL;
   }
   ~TANode() 
   {
      if ( pSid )
         delete pSid;
   }
   BOOL     Marked() { return bMarked; }
   void     Mark() { bMarked = TRUE; }
   void     SetSid(PSID p) { pSid = p; }
   void     SetName(WCHAR const * n){ safecopy(name,n); }
   WCHAR  * GetName() { return name; }
   PSID     GetSid() { return pSid;}
};

// Password generation service
#define PWGEN_MIN_LENGTH    8    // enforced minimum password length
#define PWGEN_MAX_LENGTH   14    // enforced maximum password length

struct EaPwdFilterInfo
{
   DWORD                     bEnforce;
   DWORD                     bAllowName;
   DWORD                     minLower;
   DWORD                     minUpper;
   DWORD                     minDigits;
   DWORD                     minSpecial;
   DWORD                     maxConsecutiveAlpha;
};



struct Options
{
   WCHAR                     srcComp[LEN_Account];  // source computername
   WCHAR                     srcDomain[LEN_Domain+1];
   WCHAR                     tgtDomain[LEN_Domain+1];
   WCHAR                     srcDomainDns[LEN_Path];
   WCHAR                     tgtDomainDns[LEN_Path];
   WCHAR                     tgtComp[LEN_Account];  // target computername
   PSID                      srcSid;
   PSID                      tgtSid;
   DWORD                     srcDomainVer;
   DWORD                     tgtDomainVer;
   WCHAR                     prefix[UNLEN];     // prefix for added users
   WCHAR                     suffix[UNLEN];     // suffix for added users
   WCHAR                     globalPrefix[UNLEN];
   WCHAR                     globalSuffix[UNLEN];
   WCHAR                     addToGroup[GNLEN+1]; // optional group name to add new users to
   WCHAR                     addToGroupSource[GNLEN+1]; // optional group name to add source users to
   WCHAR                     logFile[MAX_PATH+1];
   EaPwdFilterInfo           policyInfo;
   DWORD                     minPwdLength;
   CommaDelimitedLog         passwordLog;
   DWORD                     flags;             // operation flags
   BOOL                      nochange;
   WCHAR					 authUser[UNLEN+1];	//User name for source authentication
   WCHAR					 authPassword[UNLEN+1];	//Password for Authentication.
   WCHAR					 authDomain[LEN_Domain+1];	// Domain for the user passed for authentication
   HANDLE					 dsBindHandle;	// Handle to the directory service. Should be init by DsBind.
   WCHAR                     srcNamingContext[LEN_Path]; // Naming context for the Adsi path
   WCHAR                     tgtNamingContext[LEN_Path]; // Naming context for the Target domain
   WCHAR                     tgtOUPath[LEN_Path];       // path for the OU container that is to be used to create objects in
   BOOL                      expandContainers;          // Whether or not we want to expand the containers.
   BOOL                      expandMemberOf;
   BOOL                      fixMembership;
   IIManageDB              * pDb;
   BOOL                      bUndo;
   BOOL                      bSameForest;
   long                      lActionID;
   long                      lUndoActionID;
   MCSDCTWORKEROBJECTSLib::IStatusObjPtr             pStatus;
   WCHAR                     sDomUsers[UNLEN+1];               // Name of the domain users group in the source domain
   _bstr_t					 sExcUserProps;                    // user properties to exclude from migration
   _bstr_t					 sExcGroupProps;                   // group properties to exclude from migration
   _bstr_t					 sExcCmpProps;                     // computer properties to exclude from migration
   BOOL						 bExcludeProps;
   _bstr_t					 sWizard;
   

   Options() {
      srcComp[0] = 0;
      srcDomain[0] = 0;
      tgtDomain[0] = 0;
      srcDomainDns[0] = 0;
      tgtDomainDns[0] = 0;
      tgtComp[0] = 0;
      prefix[0] = 0;
      suffix[0] = 0;
      globalPrefix[0] = 0;
      globalSuffix[0] = 0;
      addToGroup[0] = 0;
      addToGroupSource[0] = 0;
      logFile[0] = 0;
      minPwdLength = 0;
      flags = 0;
      nochange = TRUE;
      authUser[0] = 0;
      authPassword[0] = 0;
      authDomain[0] = 0;
      srcNamingContext[0] = 0;
      tgtNamingContext[0] = 0;
      tgtOUPath[0] = 0;
      expandContainers = FALSE;
      fixMembership = TRUE;
      pDb = NULL;
      CoCreateInstance(CLSID_IManageDB,NULL,CLSCTX_ALL,IID_IIManageDB,(void**)&pDb);
      bUndo = FALSE;
      srcDomainVer = -1;
      tgtDomainVer = -1;
      srcSid = NULL;
      tgtSid = NULL;
      lUndoActionID = 0;
      pStatus = NULL;
      bSameForest = FALSE;
      sDomUsers[0] = 0;
	  bExcludeProps = FALSE;
   }
   ~Options()
   {
      if( pDb )
      {
         pDb->Release();
      }
      if ( srcSid )
         FreeSid(srcSid);
      if ( tgtSid )
         FreeSid(tgtSid);
   }
};


typedef void ProgressFn(WCHAR const * mesg);



int 
   UserCopy(
      Options              * options,       // in - options
      TNodeListSortable    * acctlist,      // in - list of accounts to process
      ProgressFn           * progress,      // in - function called to log current progress
      TError               & error,         // in - TError to write messages to      
      IStatusObj           * pStatus,      // in -status object to support cancellation
      void                   fn (void ),    // in - window update function
      CProcessExtensions   * pExts          // in - pointer to extensions
  );


int 
   UserRename(
      Options              * options,      // in -options
      TNodeListSortable    * acctlist,     // in -list of accounts to process
      ProgressFn           * progress,     // in -window to write progress messages to
      TError               & error,        // in -window to write error messages to
      IStatusObj           * pStatus,      // in -status object to support cancellation
      void                   WindowUpdate (void )    // in - window update function
  );

DWORD
   CopyServerName(
      WCHAR                 * uncServ     ,// out-UNC server name
      TCHAR const           * server       // in -\\server or domain name
   );


bool AddSidHistory( 
		  const Options * pOptions,
		  const WCHAR   * strSrcPrincipal,
		  const WCHAR   * strDestPrincipal,
        IStatusObj       * pStatus = NULL,
        BOOL            isFatal = TRUE
		);

bool BindToDS( 
			WCHAR * strDestDC, 
			Options * pOpt
		 );

bool AddToOU (
				  Options              * options,      // in -options
				  TNodeListSortable    * acctlist     // in -list of accounts to process
			 );

void MakeFullyQualifiedAdsPath(
                                             WCHAR * sPath,          //out- Fully qulified LDAP path to the object
											 DWORD	 nPathLen,		 //in - MAX size, in characters, of the sPath buffer
                                             WCHAR * sSubPath,       //in- LDAP subpath of the object
                                             WCHAR * tgtDomain,      //in- Domain name where object exists.
                                             WCHAR * sDN             //in- Deafault naming context for the Domain 
                                          );

BOOL GetDnsAndNetbiosFromName(WCHAR * name,WCHAR * netBios, WCHAR * dns);

void FillupNamingContext(Options * options);

bool IsAccountMigrated( 
                        TAcctReplNode * pNode,     //in -Account node that contains the Account info
                        Options       * pOptions,  //in -Options as specified by the user.
                        IIManageDBPtr   pDb,       //in -Pointer to DB manager. We dont want to create this object for every account we process
                        WCHAR         * sTgtSam    //in,out - Name of the target object that was copied if any.
                     );

bool CheckifAccountExists(
                        Options const* options,    //in-Options as set by the user
                        WCHAR * acctName           //in-Name of the account to look for
                     );

bool CallExtensions(
                     CProcessExtensions   * pExt,         // in - Extension handler.
                     Options              * options,      // in -options
                     TNodeListSortable    * acctlist,     //in -Accounts to be copied.
                     IStatusObj           * pStatus,      // in -status object to support cancellation
                     ProgressFn           * progress      //in- Progress function.
                   );

void
   CopyGlobalGroupMembers(
      Options const        * options      ,// in -options
      TAcctReplNode        * acct         ,// in -account to copy
      TNodeListSortable    * acctlist     ,// in -list of accounts being copied
      void                   WindowUpdate (void )    // in - window update function
   );

void
   CopyLocalGroupMembers(
      Options const        * options      ,// in -options
      TAcctReplNode        * acct         ,// in -account to copy
      TNodeListSortable    * acctlist     ,// in -list of accounts being copied
      void                   WindowUpdate (void )    // in - window update function
   );

HRESULT 
   CopySidHistoryProperty(
      Options              * pOptions,
      TAcctReplNode        * pNode,
      IStatusObj              * pStatus
   );

HRESULT
   GetRidPoolAllocator(
      Options              * pOptions
   );

void Mark( _bstr_t sMark,   _bstr_t sObj);
DWORD GetName(PSID pObjectSID, WCHAR * sNameAccount, WCHAR * sDomain);

typedef HRESULT (CALLBACK * ADSGETOBJECT)(LPWSTR, REFIID, void**);
extern ADSGETOBJECT            ADsGetObject;

#endif //__USERCOPY_HPP__