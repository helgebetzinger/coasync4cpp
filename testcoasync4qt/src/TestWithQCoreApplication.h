
#pragma once 

class TestWithQCoreApplication : public ::testing::Test {

protected:

	virtual void SetUp() {

		int argc = 0;
		char * argv = 0;
		mApp.reset(new QCoreApplication(argc, &argv));

	}
	virtual void TearDown() {
		mApp.reset();
	}

	bool execQtMsgLoop()
	{
		return mApp->exec();
	}
	void quitQtMsgLoop()
	{
		mApp->quit();
	}


	// DATA 

	std::shared_ptr<QCoreApplication> mApp;


};
