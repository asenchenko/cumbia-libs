#include "cuformulaparser.h"
#include <QRegularExpression>
#include <cumacros.h>

class CuFormulaParserPrivate {
public:
    std::vector<std::string> srcs;
    bool error;
    QString message;
    QString expression, formula;
    QString compiled_formula;
};

CuFormulaParser::CuFormulaParser()
{
    d = new CuFormulaParserPrivate;
    d->error = false;
}

CuFormulaParser::~CuFormulaParser()
{
    delete d;
}

bool CuFormulaParser::parse(const QString &expr)
{
    d->expression = expr;

    // regexp ^\{([\$A-Za-z0-9/,\._\-\:\s&\(\)>]+)\}\s*\((.*)\)$
    // example formula:
    // {test/device/1/double_scalar test/device/1->DevDouble(10.1)}  ( (@0+@1) * (@1 -sqrt(@0)))
    QRegularExpression re("^\\{([\\$A-Za-z0-9/,\\._\\-\\:\\s&\\(\\)>]+)\\}\\s*\\((.*)\\)$");
    QString e(d->expression);
    e.remove("formula://");
    QRegularExpressionMatch match = re.match(e);
    d->error = !match.hasMatch() || match.captured().size() < 3;
    if(!d->error) {
        QString sources = match.captured(1);
        QStringList slist;
        // split sources by space or comma
        sources.contains(" ") ?  slist = sources.split(" ", QString::SkipEmptyParts)
            : slist = sources.split(QRegExp("[\\s*,\\s*]"), QString::SkipEmptyParts);
        foreach(QString s, slist)
                d->srcs.push_back(s.toStdString());
        d->formula = match.captured(2);
    }
    else {
        d->message = QString("CuFormulaParser.parse: expression \"%1\" has no sources or formula\n"
            "example: {test/device/1/double_scalar test/device/1/long_scalar}(@0+@1)").arg(expr);
        perr("%s", qstoc(d->message));
    }
    return !d->error;
}

size_t CuFormulaParser::sourcesCount() const
{
    return d->srcs.size();
}

std::vector<std::string> CuFormulaParser::sources() const
{
    return  d->srcs;
}

std::string CuFormulaParser::source(size_t i) const
{
    if(i < d->srcs.size())
        return d->srcs.at(i);
    perr("CuFormulaParser.source: index %ld out of range", i);
    return std::string();
}

/**
 * @brief CuFormulaParser::updateSource updates the source at position i with the new
 *        source name s
 * @param i the index of the source to be updated
 * @param s the new name
 *
 * This can be used to update sources after wildcards ("$1, $2) have been replaced by
 * engine specific readers.
 *
 * \par Example
 * CuFormulaReader calls updateSource after QuWatcher::setSource so that CuFormulaParser
 * contains the complete source name, after wildcards have been replaced by QuWatcher.
 *
 * @see CuFormulaReader::setSource
 *
 */
void CuFormulaParser::updateSource(size_t i, const std::string &s)
{
    printf("\e[1;35mupdateSource replacing source at idx %zu setting to %s srcs size %zu\e[0m\n",
           i, s.c_str(), d->srcs.size());
    if(i < d->srcs.size())
        d->srcs[i] = s;
}

long int CuFormulaParser::indexOf(const std::string &src) const
{
    long int pos = std::find(d->srcs.begin(), d->srcs.end(), src) - d->srcs.begin();
    if(pos >= static_cast<long>(d->srcs.size()))
        return -1;
    return pos;
}

QString CuFormulaParser::expression() const
{
    return d->expression;
}

QString CuFormulaParser::formula() const
{
    return d->formula;
}

QString CuFormulaParser::compiledFormula() const
{
    return d->compiled_formula;
}

CuFormulaParser::State CuFormulaParser::compile(const std::vector<CuVariant> &values) const
{
    State st = CompileOk;
    QString  placeholder;
    d->message.clear();
    d->compiled_formula = d->formula;
    for(size_t i = 0; i < values.size(); i++) {
        if(values[i].isNull()) {
            st = ReadingsIncomplete;
        }
        else if(values[i].getSize() > 1) {
            st = ValueNotScalar;
        }
        else {
            bool ok;
            double to_dou = values[i].toDouble(&ok);
            if(!ok)
                return ToDoubleConversionFailed;
            else {
                placeholder = QString("@%1").arg(i);
                d->compiled_formula.replace(placeholder, QString::number(to_dou));
            }
        }
    }
    QRegularExpression re("@\\d*");
    QRegularExpressionMatch match = re.match(d->compiled_formula);
    if(match.hasMatch()) {
        d->message = QString("CuFormulaParser.compile: bad placeholder(s) in formula \"%1\":"
                             "(%2): values size is %3")
                .arg(match.captured(0))
                .arg(match.captured(1))
                .arg(values.size());
        st = CompileError;
    }

    if(st != CompileOk)
        d->compiled_formula.clear();

    return st;
}

QString CuFormulaParser::message() const
{
    return d->message;
}
