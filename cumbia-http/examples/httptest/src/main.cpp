#include <quapplication.h>
#include "httptest.h"

#include <cumbiapool.h>

#define VERSION "1.0"

int main(int argc, char *argv[])
{
    int ret;
    QuApplication qu_app( argc, argv );
    qu_app.setOrganizationName("Elettra");
    qu_app.setApplicationName("Test");
    QString version(VERSION);
    qu_app.setApplicationVersion(version);
    qu_app.setProperty("author", "Giacomo");
    qu_app.setProperty("mail", "giacomo.strangolino@elettra.eu");
    qu_app.setProperty("phone", "0403758073");
    qu_app.setProperty("office", "T2PT025");
    qu_app.setProperty("hwReferent", "$HW_REFERENT$"); /* name of the referent that provides the device server */
    

    CumbiaPool *cu_p = new CumbiaPool();
    HttpTest *w = new HttpTest(cu_p, NULL);
    w->show();

    ret = qu_app.exec();
    delete w;

//    for(std::string n : cu_p->names())
//        if(cu_p->get(n))
//            delete cu_p->get(n);

    return ret;
}
