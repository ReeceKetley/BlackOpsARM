#include "live_steam_client.h"
#include <universal/assertive.h>
#include <qcommon/common.h>
#include <DW/dwLogOn_pc.h>
#include <steam/steam_api.h>
#if defined(__ANDROID__)
#include <climits>
#endif

LiveSteamClient::LiveSteamClient()
{
    this->resultOnRequestEncryptedAppTicket = k_EResultNone;
    //CCallResult<LiveSteamClient,EncryptedAppTicketResponse_t>::CCallResult<LiveSteamClient,EncryptedAppTicketResponse_t>(&this->m_SteamCallResultEncryptedAppTicket);
    //return this;
}

// aislop
void LiveSteamClient::RequestEncryptedAppTicket(const void *data, unsigned int size)
{
    //ISteamUser *v3; // [esp+4h] [ebp-14h]
    //__int64 hSteamAPICall; // [esp+10h] [ebp-8h]

#if defined(__ANDROID__)
    if(m_SteamCallResultEncryptedAppTicket.IsActive()) m_SteamCallResultEncryptedAppTicket.Cancel();
    resultOnRequestEncryptedAppTicket=k_EResultNone;
    auto *user=SteamUser();
    if(!user || (!data && size) || size>INT_MAX) { resultOnRequestEncryptedAppTicket=k_EResultFail; return; }
    const SteamAPICall_t call=user->RequestEncryptedAppTicket(const_cast<void *>(data),static_cast<int>(size));
    if(call==k_uAPICallInvalid) { resultOnRequestEncryptedAppTicket=k_EResultFail; return; }
    m_SteamCallResultEncryptedAppTicket.Set(call,this,&LiveSteamClient::OnRequestEncryptedAppTicket);
#else
    this->resultOnRequestEncryptedAppTicket = k_EResultNone;
    if (!SteamUser()) // no-steam: no Steam client, no ticket
        return;
    // Make the Steam call and KEEP the handle
    SteamAPICall_t hSteamAPICall = SteamUser()->RequestEncryptedAppTicket((void*)data, size);

    // Unregister any previous active call result
    if (this->m_SteamCallResultEncryptedAppTicket.IsActive())
    {
        //SteamAPI_UnregisterCallResult(&this->m_SteamCallResultEncryptedAppTicket, this->m_SteamCallResultEncryptedAppTicket.m_hAPICall);
        this->m_SteamCallResultEncryptedAppTicket.Cancel();
    }

    // Store new call info
    this->m_SteamCallResultEncryptedAppTicket.Set(hSteamAPICall, this, &LiveSteamClient::OnRequestEncryptedAppTicket);
   //this->m_SteamCallResultEncryptedAppTicket.m_pObj = this;
   //this->m_SteamCallResultEncryptedAppTicket.m_Func =
   //    &LiveSteamClient::OnRequestEncryptedAppTicket;

    // Register callback if valid
    if (hSteamAPICall != k_uAPICallInvalid)
    {
        //SteamAPI_RegisterCallResult(&this->m_SteamCallResultEncryptedAppTicket, hSteamAPICall);

        this->m_SteamCallResultEncryptedAppTicket.Set(
            hSteamAPICall,
            this,
            &LiveSteamClient::OnRequestEncryptedAppTicket
        );
    }

#endif
#if 0
    //v3 = SteamUser();
    //hSteamAPICall = v3->RequestEncryptedAppTicket(v3, data, size);

    SteamUser()->RequestEncryptedAppTicket((void*)data, size);

    //if (this->m_SteamCallResultEncryptedAppTicket.m_hAPICall)
    if (this->m_SteamCallResultEncryptedAppTicket.IsActive())
        SteamAPI_UnregisterCallResult(
            &this->m_SteamCallResultEncryptedAppTicket,
            this->m_SteamCallResultEncryptedAppTicket.m_hAPICall,
            HIDWORD(this->m_SteamCallResultEncryptedAppTicket.m_hAPICall));


    this->m_SteamCallResultEncryptedAppTicket.m_hAPICall = hSteamAPICall;
    this->m_SteamCallResultEncryptedAppTicket.m_pObj = this;
    this->m_SteamCallResultEncryptedAppTicket.m_Func = LiveSteamClient::OnRequestEncryptedAppTicket;

    if (hSteamAPICall)
        SteamAPI_RegisterCallResult(&this->m_SteamCallResultEncryptedAppTicket, hSteamAPICall, HIDWORD(hSteamAPICall));
#endif
}

char __thiscall LiveSteamClient::GetRetrievedEncryptedAppTicket(
                void *ticketBuf,
                unsigned int ticketBufSize,
                unsigned int *ticketSize)
{
    ISteamUser *v5; // [esp+0h] [ebp-Ch]
    bool result; // [esp+Bh] [ebp-1h]
#if defined(__ANDROID__)
    if(ticketSize) *ticketSize=0;
    if(!ticketBuf || !ticketSize || !ticketBufSize) return 0;
#endif

    if ((this->resultOnRequestEncryptedAppTicket != k_EResultOK || !ticketBufSize)
        && !Assert_MyHandler(
            "C:\\projects_pc\\cod\\codsrc\\src\\live\\live_steam_client.cpp",
            26,
            0,
            "%s",
            "resultOnRequestEncryptedAppTicket == k_EResultOK && ticketBufSize"))
    {
        __debugbreak();
    }
    //v5 = SteamUser();
    //result = v5->GetEncryptedAppTicket(v5, ticketBuf, ticketBufSize, ticketSize);
    if (!SteamUser()) // no-steam
        return 0;
    result = SteamUser()->GetEncryptedAppTicket(ticketBuf, ticketBufSize, ticketSize);
#if defined(__ANDROID__)
    if(!result || *ticketSize>ticketBufSize) { *ticketSize=0; return 0; }
#endif
    Com_DPrintf(30, "STEAM: Retrieved ticket from Steam, sending to DemonWare\n");
    return result;
}

void __thiscall LiveSteamClient::OnRequestEncryptedAppTicket(
                EncryptedAppTicketResponse_t *pEncryptedAppTicketResponse,
                bool bIOFailure)
{
    EResult result; // [esp+4h] [ebp-4h]

#if defined(__ANDROID__)
    if(bIOFailure || !pEncryptedAppTicketResponse) { resultOnRequestEncryptedAppTicket=k_EResultFail; return; }
#endif
    if ( !bIOFailure )
    {
        result = pEncryptedAppTicketResponse->m_eResult;
        if ( pEncryptedAppTicketResponse->m_eResult == k_EResultOK )
        {
#ifdef BO1_LIVE
            dwLogonSeAcquiredSteamTicket();
#endif
        }
        else
        {
            switch ( result )
            {
                case k_EResultLimitExceeded:
                    Com_DPrintf(30, "STEAM: Calling RequestEncryptedAppTicket more than once per minute returns this error\n");
                    break;
                case k_EResultDuplicateRequest:
                    Com_DPrintf(
                        30,
                        "STEAM: Calling RequestEncryptedAppTicket while there is already a pending request results in this error\n");
                    break;
                case k_EResultNoConnection:
                    Com_DPrintf(
                        30,
                        "STEAM: Calling RequestEncryptedAppTicket while not connected to steam results in this error\n");
                    break;
            }
        }
        this->resultOnRequestEncryptedAppTicket = result;
    }
}