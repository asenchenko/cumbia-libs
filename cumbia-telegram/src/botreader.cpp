#include "botreader.h"
#include "cucontrolsreader_abs.h"
#include <cumacros.h>
#include <cumbiapool.h>
#include <cudata.h>
#include <cudataquality.h>
#include <cuformulaparsehelper.h>
#include <tsource.h>
#include <QContextMenuEvent>
#include <QMetaProperty>
#include <QScriptEngine>
#include <QRegularExpression>
#include <QtDebug>

#include "cucontrolsfactories_i.h"
#include "cucontrolsfactorypool.h"
#include "culinkstats.h"
#include "cucontextmenu.h"
#include "cucontext.h"
#include "formulahelper.h"
#include "botreadquality.h"

/** @private */
class BotReaderPrivate
{
public:
    bool auto_configure, props_only;
    bool read_ok;
    bool monitor;
    QString host, source;
    QString command;
    QString ref_mode;
    QString desc, label, unit, print_format;
    double max,min;
    CuContext *context;
    int user_id, chat_id;
    int ttl;
    int refresh_cnt, notify_cnt;
    BotReader::Priority priority;
    QDateTime startDateTime;
    int index; // to find the reader by index (shortcuts)
    CuDataQuality old_quality; // constructor sets internal quality to Valid
    CuVariant old_value;
    bool formula_is_identity;
};

/** \brief Constructor with the parent widget, *CumbiaPool*  and *CuControlsFactoryPool*
 *
 *   Please refer to \ref md_src_cumbia_qtcontrols_widget_constructors documentation.
 */
BotReader::BotReader(int user_id,
                     int chat_id,
                     QObject *w,
                     CumbiaPool *cumbia_pool,
                     const CuControlsFactoryPool &fpool,
                     int ttl,
                     const QString &cmd,
                     Priority pri,
                     const QString &host,
                     bool monitor) :
    QObject(w), CuDataListener()
{
    m_init();
    d->context = new CuContext(cumbia_pool, fpool);
    d->chat_id = chat_id;
    d->user_id = user_id;
    d->ttl = ttl;
    d->priority = pri;
    d->monitor = monitor;
    d->command = cmd;
    d->index = -1;
    const char* env_tg_host = nullptr;
    if(host.isEmpty() && (env_tg_host = secure_getenv("TANGO_HOST")) != nullptr) {
        d->host = QString(env_tg_host);
    }
    else
        d->host = host;
}

void BotReader::m_init()
{
    d = new BotReaderPrivate;
    d->context = NULL;
    d->auto_configure = true;
    d->props_only = false;
    d->read_ok = false;
    d->refresh_cnt = d->notify_cnt = 0;
    d->max = d->min = 0.0;
    d->old_quality = CuDataQuality::Undefined;
    d->formula_is_identity = false;
}

BotReader::~BotReader()
{
    printf("\e[1;31m~BotReader %p\e[0m\n", this);
    delete d->context;
    delete d;
}

void BotReader::setPropertyEnabled(bool get_props)
{
    d->auto_configure = get_props;
}

void BotReader::setPropertiesOnly(bool props_only)
{
    d->props_only = props_only;
}

/**
 * @brief BotReader::sources returns the list of sources identified in d->source
 *
 * @return a list of sources identified in d->source. If d->source is a formula,
 *         more than one source may be involved. Otherwise, the method returns the
 *         only source set with setSource
 */
QStringList BotReader::sources() const
{
    CuFormulaParseHelper ph;
    return ph.sources(d->source);
}

QString BotReader::source() const
{
    return d->source; // without host
}

/**
 * @brief BotReader::hasSource returns true if the reader sources contain src, false otherwise
 * @param src the source to search for
 * @return true if src in sources, false otherwise
 *
 * @see sources
 */
bool BotReader::hasSource(const QString &src) const
{
    return sources().contains(src);
}

/**
 * @brief BotReader::sourceMatch finds if one source matches the given patter (regexp match)
 * @param pattern the regexp pattern
 * @return true if *one of* the sources matches pattern
 *
 * \par Example
 * \code
 * reader->setSource("{test/device/1/long_scalar, test/device/2/double_scalar}(@1 + @2)");
 * bool match = sourceMatch("double_scalar");
 * // match is true
 *
 * \endcode
 */
bool BotReader::sourceMatch(const QString &pattern) const
{
    return CuFormulaParseHelper().sourceMatch(d->source, pattern);
}

bool BotReader::sameSourcesAs(const QSet<QString> &srcset) const
{
    return sources().toSet() == srcset;
}

/**
 * @brief BotReader::formulaIsIdentity returns true if the source of the reader does not
 *        contain a formula, such as "test/device/1/double_scalar", false otherwise
 *
 * @return true: this is a plain reader of a single source, no further calculations are made
 * @return false: this is a reader with one or more sources and the result implies a calculation,
 *         for example  test/device/1/double_scalar - 2 * test/device/2/double_scalar
 */
bool BotReader::hasNoFormula() const
{
    return d->formula_is_identity;
}

QString BotReader::command() const
{
    return d->command;
}

void BotReader::setCommand(const QString &cmd)
{
    d->command = cmd;
}

QString BotReader::host() const
{
    return d->host;
}

QDateTime BotReader::startedOn() const
{
    return d->startDateTime;
}

int BotReader::ttl() const
{
    return d->ttl;
}

void BotReader::setStartedOn(const QDateTime &dt)
{
    if(d->startDateTime.isValid()) {
        perr("BotReader.setStartedOn: cannot set start date time more than once");
    }
    else {
        d->startDateTime = dt;
    }
}

bool BotReader::hasStartDateTime() const
{
    return d->startDateTime.isValid();
}

/** \brief returns the pointer to the CuContext
 *
 * CuContext sets up the connection and is used as a mediator to send and get data
 * to and from the reader.
 *
 * @see CuContext
 */
CuContext *BotReader::getContext() const
{
    return d->context;
}

int BotReader::userId() const
{
    return d->user_id;
}

int BotReader::chatId() const
{
    return d->chat_id;
}

BotReader::Priority BotReader::priority() const
{
    return d->priority;
}

/**
 * @brief BotMonitor::index useful to find a Reader by its unique index
 *
 * The purpose of an index is to create a "/shortcut" to easily find a
 * reader by its unique id.
 *
 * The id is unique for every reader *per user_id*
 *
 * @return the BotReader index
 */
int BotReader::index() const
{
    return d->index;
}

void BotReader::setIndex(int idx)
{
    d->index = idx;
}

int BotReader::refreshCount() const
{
    return  static_cast<int>(d->refresh_cnt);
}

int BotReader::notifyCount() const
{
    return d->notify_cnt;
}

QString BotReader::refreshMode() const
{
    return d->ref_mode;
}

QString BotReader::print_format() const
{
    return d->print_format;
}

double BotReader::min() const
{
    return  d->min;
}

QString BotReader::description() const
{
    return d->desc;
}

QString BotReader::label() const
{
    return d->label;
}

void BotReader::setPriority(BotReader::Priority pri)
{
    if(pri != d->priority) {
        Priority oldpri = d->priority;
        d->priority = pri;
        emit priorityChanged(d->chat_id, source(), oldpri, d->priority);
    }
}

/** \brief Connect the reader to the specified source.
 *
 * If a reader with a different source is configured, it is deleted.
 * If options have been set with QuContext::setOptions, they are used to set up the reader as desired.
 *
 * @see QuContext::setOptions
 * @see source
 */
void BotReader::setSource(const QString &s)
{
    CuData options;
    if(!d->auto_configure)
        options["no-properties"] = true;
    if(d->props_only)
        options["properties-only"] = true;
    if(d->monitor)
        options["period"] = 15000;
    if(!options.isEmpty())
        d->context->setOptions(options);

    QString src(s);
    CuControlsReaderA * r = d->context->replace_reader(src.toStdString(), this);
    if(r)
        r->setSource(src);

    // no host in source()
    d->source = s;
    CuFormulaParseHelper ph;
    d->formula_is_identity = ph.identityFunction(d->source);
    printf("\e[1;34mBotReader.setSource: src %s IS IDENTITY %d\n", qstoc(d->source), d->formula_is_identity);
}

void BotReader::unsetSource()
{
    d->context->disposeReader();
}

/**
 * @brief BotReader::setFormula set or change the formula
 * @param formula the new formula
 */
void BotReader::setFormula(const QString &formula)
{
    if(formula != d->command) {
        const QString old_f = d->command;
        d->command = formula;
        emit formulaChanged(d->chat_id, source(), old_f, formula);
    }
}

void BotReader::m_configure(const CuData& da)
{
    d->unit = QString::fromStdString(da["display_unit"].toString());
    d->label = QString::fromStdString(da["label"].toString());
    d->desc = QString::fromStdString(da["description"].toString());
    d->min = da["min"].toDouble();
    d->max = da["max"].toDouble();
    d->print_format = QString::fromStdString(da["format"].toString());
}

/**
 * @brief BotReader::onUpdate manages new data from the engine
 *
 * If it is the first call and there is no error condition, startSuccess is emitted.
 * startSuccess signal will contain the user and chat ids,
 * the source name (host name excluded) and the formula.
 *
 * @param da
 */
void BotReader::onUpdate(const CuData &da)
{
    d->read_ok = !da["err"].toBool();
    if(d->read_ok && d->refresh_cnt == 0) {
        m_check_or_setStartedNow(); // read method comments
        emit startSuccess(d->user_id, d->chat_id, source(), d->command);
    }

    // configure object if the type of received data is "property"
    if(d->read_ok && d->auto_configure && da["type"].toString() == "property") {
        m_configure(da);
    }
    // in case of error: quit
    // in case we got a value: quit
    // if monitor, m_publishResult is always called
    if(da.containsKey("value") || !d->read_ok ) {
        // evaluate formula, if present, and emit newData
        bool success = m_publishResult(da);
        if(!d->monitor || !success) {
            deleteLater();
        }
    }
    d->refresh_cnt++;
    if(!d->startDateTime.isValid() || d->startDateTime.secsTo(QDateTime::currentDateTime()) >= d->ttl)
        emit lastUpdate(d->chat_id, da);
    if(da.containsKey("mode"))
        d->ref_mode = QString::fromStdString(da["mode"].toString());
}

bool BotReader::m_publishResult(const CuData &da)
{
    CuData data(da);
    bool success = !da["err"].toBool();
    bool value_changed;
    bool notify = false;
    bool is_alert = (d->priority == BotReader::Normal);
    data["silent"] = (d->priority == Low);
    data["index"] = d->index;
    data["print_format"] = d->print_format.toStdString();
    data["display_unit"] = d->unit.toStdString();
    data["command"] = d->command.toStdString();
    CuDataQuality read_quality(da["quality"].toInt());
    if(!success) {
        read_quality.set(CuDataQuality::Invalid);
    }

    // extract value
    CuVariant v = da["value"];
    value_changed = (v != d->old_value || read_quality != d->old_quality);

    printf("\e[1;33mBotReader value cahnged %d cuz old value %s new %s old q %d new q %d\e[0m\n",
           value_changed, v.toString().c_str(), d->old_value.toString().c_str(),
           d->old_quality.toInt(), read_quality.toInt());

    if(value_changed && !d->formula_is_identity) {
        // data contains the result of a formula
        bool is_bool = v.getType() == CuVariant::Boolean && v.getFormat() == CuVariant::Scalar;
        notify = true;
        if(is_bool)
            v.toBool() ? data["evaluation"] = std::string("yes") : data["evaluation"] = std::string("no");
        if(d->monitor && is_bool && v.toBool()) { // formula evaluates to boolean and it is true
            // (for instance, test/device/1/double_scalar > 250 evaluates to bool)
            // monitor type: "monitor" or "alert"
            // "alert": notify on quality change only
            // "monitor": notify (silently, with low pri) always
            read_quality.set(CuDataQuality::Alarm);
            notify =  (d->priority == BotReader::Normal && d->old_quality != read_quality)
                    || d->priority == BotReader::Low;
        }
        else if(d->monitor && !is_bool) {
            // formula result is the result of a calculation, does not evaluate to bool
            // so publish the result if either
            // - "monitor" priority Low (silent message)
            // or
            // - "alert" priority Normal (message + notification): only on quality changed
            //
            notify = !is_alert || (is_alert && d->old_quality != read_quality);
        }
    }
    else if(value_changed) { // no formula
        notify = !is_alert || (is_alert && d->old_quality != read_quality);
    }

    if(value_changed) {
        d->old_value = v;
        printf("\e[0;33mducling old q %d new %d\e[0m\n", d->old_quality.toInt(), read_quality.toInt());
        d->old_quality = read_quality;
        printf("\e[0;33mAFTER EQUALLING old q %d new %d\e[0m\n", d->old_quality.toInt(), read_quality.toInt());
    }

    if(notify) {
        emit newData(d->chat_id, data);
        d->notify_cnt++;
    }
    return success;
}

void BotReader::m_check_or_setStartedNow()
{
    if(!d->startDateTime.isValid()) // check if not already set
        d->startDateTime = QDateTime::currentDateTime();
}
