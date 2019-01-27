#include <bio/Biosphere>
#include <string>

// bio.launch - PoC qlaunch replacement, and a good example of using Biosphere :)

int main()
{
    // We have to override the heap to launch library applets, Biosphere handles it internally
    bio::os::OverrideHeap(0x10000000).AssertOk();

    // 'app' module initializes sm and applet services, we initialize as a system applet as we're qlaunch in this case
    bio::app::Initialize(bio::app::RunMode::SystemApplet).AssertOk();

    // Wait a second to let applet init everything
    bio::svc::SleepThread(1000000000);

    // Get all the service objects we'll need
    bio::applet::ae::SystemAppletProxy *isap = (bio::applet::ae::SystemAppletProxy*)bio::app::GetProxyObject();
    bio::applet::WindowController *wc = isap->GetWindowController().AssertOk();
    bio::applet::HomeMenuFunctions *hmf = isap->GetHomeMenuFunctions().AssertOk();
    bio::applet::ApplicationCreator *ac = isap->GetApplicationCreator().AssertOk();
    bio::applet::LibraryAppletCreator *lac = bio::app::GetLibraryAppletCreator();

    // Using Biosphere's high-level library applet calling system we call 'playerSelect' applet to select a user...
    bio::app::PlayerSelectApplet *psel = new bio::app::PlayerSelectApplet();
    // ...and we get the Uid, aka u128...
    bio::account::Uid userid = psel->Show().AssertOk();
    // ...and finalize the applet
    delete psel;

    // Create the application accessor, in this case hardcoding SSBU
    bio::applet::ApplicationAccessor *aa = ac->CreateApplication(bio::ApplicationId(0x01006A800016E000)).AssertOk();

    // Get state changed event (is this neccessary?)
    bio::os::Event *chev = aa->GetAppletStateChangedEvent().AssertOk();

    // Via Home Menu functions, unlock foreground (is this neccessary?)
    hmf->UnlockForeground().AssertOk();

    // Argument struct to send as parameter for titles which require an user
    // To be more precise, we should get the NACP data from the application accessor and check whether it requires an user, but SSBU does require it
    struct UserData
    {
        u32  m;
        u8   unk1;
        u8   pad[3];
        u128 userID;
        u8   unk2[0x70];
    } BIO_PACKED;

    // Create and populate the data
    UserData udata = { 0 };
    udata.m = 0xc79497ca;
    udata.unk1 = 1;
    udata.userID = userid;

    // Create the storage, write the data and close the accessor after writing the data
    bio::applet::Storage *ust = lac->CreateStorage(sizeof(UserData)).AssertOk();
    bio::applet::StorageAccessor *usta = ust->Open().AssertOk();
    usta->Write(0, &udata, sizeof(UserData)).AssertOk();
    delete usta;

    // Push the paramater storage with the specific parameter type
    aa->PushLaunchParameter(bio::applet::ParameterKind::SelectedUser, ust).AssertOk();

    // Request to get foreground
    aa->RequestForApplicationToGetForeground().AssertOk();

    // Start the title
    aa->Start().AssertOk();

    // Wait the state changed event (is this neccessary at all?)
    chev->Wait(UINT64_MAX).AssertOk();
    delete chev;

    // Infinite loop, qlaunch should never end
    while(true);

    // Finalize applet and sm (despite that the services we manually accessed should be closed before calling this, but this point is never reached)
    bio::app::Finalize();
    return 0;
}