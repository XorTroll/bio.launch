#include <bio/Biosphere>
#include <string>

// bio.launch

int main()
{
    bio::os::OverrideHeap(0x10000000).AssertOk();
    bio::app::Initialize(bio::app::RunMode::SystemApplet).AssertOk();
    bio::svc::SleepThread(1000000000);

    bio::applet::ae::SystemAppletProxy *isap = (bio::applet::ae::SystemAppletProxy*)bio::app::GetProxyObject();
    bio::applet::WindowController *wc = isap->GetWindowController().AssertOk();
    bio::applet::HomeMenuFunctions *hmf = isap->GetHomeMenuFunctions().AssertOk();
    bio::applet::ApplicationCreator *ac = isap->GetApplicationCreator().AssertOk();
    bio::applet::LibraryAppletCreator *lac = bio::app::GetLibraryAppletCreator();

    bio::app::PlayerSelectApplet *psel = new bio::app::PlayerSelectApplet();
    bio::account::Uid userid = psel->Show().AssertOk();

    bio::applet::ApplicationAccessor *aa = ac->CreateApplication(bio::ApplicationId(0x01006A800016E000)).AssertOk();
    bio::os::Event *chev = aa->GetAppletStateChangedEvent().AssertOk();
    hmf->UnlockForeground().AssertOk();

    struct UserData
    {
        u32  m;
        u8   unk1;
        u8   pad[3];
        u128 userID;
        u8   unk2[0x70];
    } BIO_PACKED;
    UserData udata = { 0 };
    udata.m = 0xc79497ca;
    udata.unk1 = 1;
    udata.userID = userid;

    bio::applet::Storage *ust = lac->CreateStorage(sizeof(UserData)).AssertOk();
    bio::applet::StorageAccessor *usta = ust->Open().AssertOk();
    usta->Write(0, &udata, sizeof(UserData)).AssertOk();
    delete usta;
    aa->PushLaunchParameter(bio::applet::ParameterKind::SelectedUser, ust).AssertOk();

    aa->RequestForApplicationToGetForeground().AssertOk();
    aa->Start().AssertOk();

    chev->Wait(UINT64_MAX).AssertOk();
    delete chev;

    while(true);

    bio::app::Finalize();
    return 0;
}