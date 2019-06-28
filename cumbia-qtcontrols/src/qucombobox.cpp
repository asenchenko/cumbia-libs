#include "qucombobox.h"
#include "cucontrolswriter_abs.h"
#include <cumacros.h>
#include <cumbiapool.h>
#include <cudata.h>
#include <QContextMenuEvent>
#include <QMetaProperty>
#include <QStringList>
#include <vector>

#include <cucontrolsfactories_i.h>
#include <cucontrolsfactorypool.h>
#include <culinkstats.h>
#include <cucontextmenu.h>
#include <cucontext.h>
#include <culog.h>
#include <cuserviceprovider.h>
#include <cuservices.h>
#include <qulogimpl.h>

/** @private */
class QuComboBoxPrivate
{
public:
    bool auto_configure;
    bool ok;
    bool index_mode, execute_on_index_changed;
    CuContext *context;

    // for private use
    bool configured, index_changed_connected;
};

/** \brief Constructor with the parent widget, an *engine specific* Cumbia implementation and a CuControlsReaderFactoryI interface.
 *
 *  Please refer to \ref md_src_cumbia_qtcontrols_widget_constructors documentation.
 */
QuComboBox::QuComboBox(QWidget *w, Cumbia *cumbia, const CuControlsWriterFactoryI &r_factory) :
    QComboBox(w), CuDataListener()
{
    m_init();
    d->context = new CuContext(cumbia, r_factory);
    std::vector<std::string> vs = { "values" };
    d->context->setOptions(CuData("fetch_props", vs ));
}

/** \brief Constructor with the parent widget, *CumbiaPool*  and *CuControlsFactoryPool*
 *
 *   Please refer to \ref md_src_cumbia_qtcontrols_widget_constructors documentation.
 */
QuComboBox::QuComboBox(QWidget *w, CumbiaPool *cumbia_pool, const CuControlsFactoryPool &fpool) :
    QComboBox(w), CuDataListener()
{
    m_init();
    d->context = new CuContext(cumbia_pool, fpool);
    std::vector<std::string> vs = { "values" };
    d->context->setOptions(CuData("fetch_props", vs ));
}

void QuComboBox::m_init()
{
    d = new QuComboBoxPrivate;
    d->context = NULL;
    d->auto_configure = true;
    d->ok = false;
    d->index_mode = false;
    d->execute_on_index_changed = false;
    d->configured = d->index_changed_connected = false; // for private use
}

QuComboBox::~QuComboBox()
{
    pdelete("~QuComboBox %p", this);
    delete d->context;
    delete d;
}

QString QuComboBox::target() const {
    CuControlsWriterA *w = d->context->getWriter();
    if(w != NULL)
        return w->target();
    return "";
}

/** \brief returns the pointer to the CuContext
 *
 * CuContext sets up the connection and is used as a mediator to send and get data
 * to and from the reader.
 *
 * @see CuContext
 */
CuContext *QuComboBox::getContext() const
{
    return d->context;
}

/*! \brief returns index mode enabled or not
 *
 * @return true: the combo box is working in index mode
 * @return false: the combo box is working with its text items
 *
 * When index mode is enabled, the current item is chosen by its index
 * at configuration time and the value used for writing is the QComboBox currentIndex.
 * When it is disabled, currentText is used for writings and the current item
 * at configuration time is searched by the source value (string comparison).
 */
bool QuComboBox::indexMode() const
{
    return  d->index_mode;
}

/*! \brief returns the value of the property executeOnIndexChanged
 *
 * @return true when the index of the combo box changes, a write operation is sent through the link
 * @return false no writings are executed upon index change
 *
 * @see setExecuteOnIndexChanged
 */
bool QuComboBox::executeOnIndexChanged() const
{
    return  d->execute_on_index_changed;
}

QString QuComboBox::getData() const
{
    if(d->index_mode)
        return QString::number(currentIndex());
    return currentText();
}

/** \brief Connect the reader to the specified source.
 *
 * If a reader with a different source is configured, it is deleted.
 * If options have been set with QuContext::setOptions, they are used to set up the reader as desired.
 *
 * @see QuContext::setOptions
 * @see source
 */
void QuComboBox::setTarget(const QString &target)
{
    CuControlsWriterA* w = d->context->replace_writer(target.toStdString(), this);
    if(w)
        w->setTarget(target);
}

void QuComboBox::contextMenuEvent(QContextMenuEvent *e)
{
    CuContextMenu* m = new CuContextMenu(this, d->context);
    connect(m, SIGNAL(linkStatsTriggered(QWidget*, CuContextI *)),
            this, SIGNAL(linkStatsRequest(QWidget*, CuContextI *)));
    m->popup(e->globalPos());
}

void QuComboBox::m_configure(const CuData& da)
{
    d->ok = !da["err"].toBool();

    setDisabled(!d->ok);

    if(d->ok) {
        QString description, unit, label;
        CuVariant items;

        unit = QString::fromStdString(da["display_unit"].toString());
        label = QString::fromStdString(da["label"].toString());
        description = QString::fromStdString(da["description"].toString());

        setProperty("description", description);
        setProperty("unit", unit);

        if(da.containsKey("values")) {
            const std::vector<std::string> labels = da["values"].toStringVector();
            for(size_t i = 0; i < labels.size(); i++)
                insertItem(static_cast<int>(i), QString::fromStdString(labels[i]));
        }

        // initialise the object with the "write" value (also called "set point"), if available:
        //
        if(da.containsKey("w_value")) {
            bool ok;
            int index = -1;
            if(d->index_mode) {
                ok = da["w_value"].to<int>(index);
                if(ok && index > -1) { // conversion successful
                    setCurrentIndex(index);
                }
            }
            else {
                std::string as_s = da["w_value"].toString(&ok);
                if(ok) {
                    index = findText(QString::fromStdString(as_s));
                    if(index > -1)
                        setCurrentIndex(index);
                }
            }
        }

        QString toolTip = QString::fromStdString(da["msg"].toString()) + "\n";
        if(!label.isEmpty()) toolTip += label + " ";
        if(!unit.isEmpty()) toolTip += "[" + unit + "] ";
        if(!description.isEmpty()) toolTip += "\n" + description;

        setToolTip(toolTip);

        if(d->execute_on_index_changed && !d->index_changed_connected) {
            d->index_changed_connected = true;
            connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(m_onIndexChanged(int)));
        }
        d->configured = true;
    }
    else { // !ok
        setToolTip(da["msg"].toString().c_str());
    }
}


void QuComboBox::onUpdate(const CuData &da)
{
    d->ok = !da["err"].toBool();
    if(!d->ok) {
        perr("QuComboBox [%s]: error %s target: \"%s\" format %s (writable: %d)", qstoc(objectName()),
             da["src"].toString().c_str(), da["msg"].toString().c_str(),
                da["data_format_str"].toString().c_str(), da["writable"].toInt());

        Cumbia* cumbia = d->context->cumbia();
        if(!cumbia) /* pick from the CumbiaPool */
            cumbia = d->context->cumbiaPool()->getBySrc(da["src"].toString());
        CuLog *log;
        if(cumbia && (log = static_cast<CuLog *>(cumbia->getServiceProvider()->get(CuServices::Log))))
        {
            static_cast<QuLogImpl *>(log->getImpl("QuLogImpl"))->showPopupOnMessage(CuLog::Write, true);
            log->write(QString("QuApplyNumeric [" + objectName() + "]").toStdString(), da["msg"].toString(), CuLog::Error, CuLog::Write);
        }
    }
    else if(d->auto_configure && da["type"].toString() == "property") {
        //
        // --------------------------------------------------------------------------------------------
        // You may want to check data format and write type and issue a warning or avoid configuration
        // at all if they are not as expected
        // if(da["data_format_str"] == "scalar" && da["writable"].toInt() > 0)
        // --------------------------------------------------------------------------------------------
        m_configure(da);
    }
    emit newData(da);
}

void QuComboBox::m_onIndexChanged(int i) {
    printf("\e[1;31mQuComboBox.m_onIndexChanged: new index %d \e[0m\n", i);
    if(d->index_mode) {
        printf("\e[0;31mWILL WRITE BY INDEX: %d\e[0m\n", i);
        write(i);
    }
    else {
        printf("\e[0;33mWILL WRITE BY text: %s\e[0m\n", qstoc(currentText()));
        write(currentText());
    }
}

/** \brief write an integer to the target
 *
 * @param i the value to be written on the target
 */
void QuComboBox::write(int ival) {
    m_write(CuVariant(ival));
}

/** \brief write a string to the target
 *
 * @param s the string to be written on the target
 */
void QuComboBox::write(const QString& s) {
    m_write(CuVariant(s.toStdString()));
}

/*! \brief enable or disable index mode
 *
 * @param im true index mode enabled
 * @param im false index mode disabled
 * @see indexMode
 */
void QuComboBox::setIndexMode(bool im) {
    d->index_mode = im;
}

/*! \brief change the executeOnIndexChanged property
 *
 * @param exe true: write on index change according to indexMode
 * @param exe false (default) noop on index change
 *
 * @see executeOnIndexChanged
 */
void QuComboBox::setExecuteOnIndexChanged(bool exe)
{
    d->execute_on_index_changed = exe;
    if(!exe) {
        disconnect(this, SIGNAL(currentIndexChanged(int)));
        d->index_changed_connected = false;
    }
    if(d->configured && !d->index_changed_connected && exe) {
        d->index_changed_connected = true;
        connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(m_onIndexChanged(int)));
    }
}

// perform the write operation on the target
//
void QuComboBox::m_write(const CuVariant& v){
    CuControlsWriterA *w = d->context->getWriter();
    if(w) {
        w->setArgs(v);
        w->execute();
    }
}
