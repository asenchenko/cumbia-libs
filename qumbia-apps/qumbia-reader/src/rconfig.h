#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QStringList>

class RConfig
{
public:
    RConfig();
    int refresh_limit;
    int truncate, max_timers, verbosity;
    QStringList sources;
    int period;
    bool usage, list_options, property, help, no_properties;

    void setPropertyOnly();
    QString format;
    QString unchanged_upd_mode; // always, timestamp, none
    QString db_profile, db_output_file; // if historical db plugin available

    // websocket
    QString url;
};

#endif // CONFIGURATION_H
