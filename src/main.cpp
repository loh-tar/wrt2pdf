//
//  wrt2pdf - Create a PDF out of a plain text file
//
//  Copyright (C) 2022 loh.tar@googlemail.com
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
//  MA 02110-1301, USA.


#define MY_NAME "wrt2pdf"
#define MY_VERSION "0.6"
#define MY_FONTFAMILY "Hack"
#define MY_FONTSIZE 10

//  My start point for this little project
//    https://wiki.qt.io/Exporting_a_document_to_PDF
//
//  An alternative would be to handle the printing on your own
//    https://doc.qt.io/qt-5/qprinter.html
//
// TODO
// - Force to use QStringLiteral/QLatin1String by QT_NO_CAST TO/FROM ASCII
//   https://doc.qt.io/qt-5/qstring.html#QStringLiteral
// - Perhaps make translations possible after all?

#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFontInfo>
#include <QFontMetricsF>
#include <QTextDocument>
#include <QTextStream>
#ifndef QT_NO_PRINTER
#include <QPrinter>
#endif

// https://newbedev.com/how-to-print-to-console-when-using-qt
inline QTextStream& qStdOut()
{
    static QTextStream r{stdout};
    return r;
}

inline QTextStream& qStdErr()
{
    static QTextStream r{stderr};
    return r;
}

inline qreal mmToPoints(const qreal& mm) {
    return (mm * 72/25.4);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(MY_NAME);
    QCoreApplication::setApplicationVersion(MY_VERSION);

    QCommandLineParser parser;

    // We don't use "translate" stuff, 1) I'm too lazy 2) localized help text is more a pain than a plus
    parser.addPositionalArgument("pdf-to-create", "The suffix .pdf will be added automatically when missing", "[pdf-to-create]");
    parser.addPositionalArgument("text-file", "File to be converted. When not given stdin is used", "[text-file]");
    // Qt doku says: C++11-style uniform initialization
    parser.addOption({{"F", "force"}, "Overwrite existing file [pdf-to-create]"});
    parser.addOption({{"i", "in-file"}, "File to be converted. When no [pdf-to-create] is given <file-name> is used with .pdf suffix", "file-name"});
    parser.addOption({{"f", "font"}, "Set the font to use by description", "font-desc"});
    parser.addOption({{"L", "list-fonts"}, "List available fixed pitch fonts"});
    parser.addOption({{"m", "margins"}, "Set the page margins in millimeter as string 'left,right,top,bottom'", "l,r,t,b", "5.0,5.0,5.0,5.0"});
    parser.addOption({{"p", "page-size"}, "Set the paper size by PPD media option keyword", "mok"});
    parser.addOption({{"P", "list-mo-keys"}, "List PPD media option keywords (mok) and description", "key-filter"});
    parser.addOption({{"l", "landscape"}, "Use page in landscape orientation"});
    parser.addOption({{"I", "info"}, "Like a dry-run, shows settings and resulting page size in rows/cols"});
    parser.addOption({{"T", "test-page"}, "Generate a test page to verify intended settings, similar to -I"});
    parser.addVersionOption(); // Argh, this shows localized help text FIXME
    // We don't use Qt build-in help option
    parser.addOption({{"h", "?"}, "Show usage"});
    parser.addOption({{"H", "help"}, "Show usage, examples and some more hints"});
    //parser.addOption(moreHelpOption);

    // Process the actual command line arguments given by the user
    parser.process(app);

    //
    // Let's get ready to rumble!
    // Show any kind of help first before we need to dig deeper
    //

    // Show BIG help first..
    if (parser.isSet("help")) {
        // moreHelpOption.setDescription("You read it NOW"); // FIXME Don't work!(?)

        // May better, but so many warnings: https://doc.qt.io/qt-5/qcoreapplication.html#arguments
        // const QString me = app.arguments().at(0);
        const QString me = app.applicationName();
        qStdOut() <<  "This is " MY_NAME " v" MY_VERSION << Qt::endl
                  << "Create a PDF out of a plain text file" << Qt::endl
                  << Qt::endl
                  << parser.helpText() << Qt::endl
                  << "Examples:" << Qt::endl
                  << "  Create ./foo.pdf out of /some/where/bar on US Letter" << Qt::endl
                  << "      " << me << " -p letter foo /some/where/bar " << Qt::endl
                  << Qt::endl
                  << "  Make a PDF from this help text (funny line, huh?)" << Qt::endl
                  << "      " << me << " --help | " << me << " " << me << "-help" << Qt::endl
                  << Qt::endl
                  << "  Create /some/where/bar.pdf out of /some/where/bar.txt with a custom 10.5mm" << Qt::endl
                  << "  left margin and 20mm top margin" << Qt::endl
                  << "      " << me << " --margins 10.5,,20  -i /some/where/bar.txt" << Qt::endl
                  << Qt::endl
                  << "Note: You can omit margins, then is the default of 5mm used" << Qt::endl
                  << Qt::endl
                  << "  Use custom font and size by --font option" << Qt::endl
                  << "      " << me << " -f 'Source Code Pro,Light,11' -i foo.txt" << Qt::endl
                  << "      " << me << " -f 'Helvetica [Cronyx],10' -i foo.txt" << Qt::endl
                  << "      " << me << " -i foo.txt -f 'Helvetica-Cronyx,10'" << Qt::endl
                  << Qt::endl
                  << "Note: The first request the font in style Light and size 11Points. The latter" << Qt::endl
                  << "      two are equal and demonstrate that options may appear anywhere." << Qt::endl
                  << "Furthermore is there no special font-style requested but both ways shown how to" << Qt::endl
                  << "give a foundry (Cronyx in this case)" << Qt::endl
                  << Qt::endl
                  << "Miscellaneous:" << Qt::endl
                  << "  - The hard coded default paper is A4" << Qt::endl
                  << "  - The hard coded default font is Hack in size 10Points" << Qt::endl
                  << "  - When using -i without [pdf-to-create] there is no override check done" << Qt::endl
                  << "  - Fonts displayed by -L prove to be quite unreliable. Some are mysteriously" << Qt::endl
                  << "    replaced when selected and some have no fixed pitch, resulting in incorrect" << Qt::endl
                  // e.g. "Monospace" and "Noto Sans SignWriting"
                  << "    calculations of maximum rows and cols" << Qt::endl
                  << "  - The key given by --page-size must match exactly but is case insensitive" << Qt::endl
                  ;
        return 0;
    }

    // ...and normal help if no BIG help was requested
    if (parser.isSet("h")) {
      parser.showHelp(0);
    }

    // Font listing is another kind of help...
    if (parser.isSet("list-fonts")) {
        QFontDatabase database;
        const QStringList fontFamilies = database.families();
        for (const QString &family : fontFamilies) {

            if (database.isPrivateFamily(family)) continue; // Apple systems only, says docu
            if (! database.isFixedPitch(family)) continue;  // For my taste should we only use fixed fonts
            if (! database.isScalable(family)) continue;    // Um, "Terminus" e.g. don't work, will use: lines=423 columns=599

            qStdOut() << family << Qt::endl;

            const QStringList fontStyles = database.styles(family);
            for (const QString &style : fontStyles) {
                if (! database.isScalable(family, style)) continue;

                QString sizes;
                const QList<int> pointSizes = database.pointSizes(family, style);
                for (int points : pointSizes)
                    sizes += QString::number(points) + ' ';

                qStdOut() << "  " << style << " : " << sizes.trimmed() << Qt::endl;
            }
        }
        return 0;
    }

    // Some needed var
    QString pdfFile;
    QString txtFile;
    QString txtFileName;
    QString fontFamily = MY_FONTFAMILY;
    QString fontStyle;
    int fontSize = MY_FONTSIZE;
    const qreal defautMargin = 5.0;
    QPageSize pageSize = QPageSize(QPageSize::A4);
    QPageLayout::Orientation pageOrientation = QPageLayout::Portrait;
    QList<qreal> marginList;
    enum MarginType {
        LeftMargin,
        RightMargin,
        TopMargin,
        BottomMargin
    };

    // ...just as paper listing
    if (parser.isSet("list-mo-keys")) {
        const QString filter = parser.value("list-mo-keys");
        // qStdOut() << QString("%1   %2").arg("Key", -18).arg("Description") << Qt::endl;

        for (int id = 0; ; ++id) {
            const QString merger = QPageSize::key(static_cast<QPageSize::PageSizeId>(id)) + QPageSize::name(static_cast<QPageSize::PageSizeId>(id));

            if (merger.isEmpty()) break;
            if (!merger.contains(filter, Qt::CaseInsensitive)) continue;
            if (merger.startsWith("Custom", Qt::CaseInsensitive)) continue; // We don't support custom sizes

            qStdOut() << QString("%1 : %2")
                        .arg(QPageSize::key(static_cast<QPageSize::PageSizeId>(id)), -18)
                        .arg(QPageSize::name(static_cast<QPageSize::PageSizeId>(id)))
                      << Qt::endl;
        }

        return 0;
    }

    //
    // Below this point we need to investigate all settings in detail before we can do something useful
    // We start with page and font settings...
    //

    if (parser.isSet("page-size")) {
        const QString match = parser.value("page-size");
        for (int id = 0; ; ++id) {
            const QString key = QPageSize::key(static_cast<QPageSize::PageSizeId>(id));

            if (key.isEmpty()) {
                qStdErr() << "Key not found: " << match << Qt::endl;
                return 1;
            }
            if (key.compare(match, Qt::CaseInsensitive)) continue;
            pageSize= QPageSize(static_cast<QPageSize::PageSizeId>(id));
            break;
        }
    }

    if (parser.isSet("landscape")) {
        pageOrientation = QPageLayout::Landscape;
    }

    // Examine font setting
    if (parser.isSet("font")) {
        fontFamily.clear();
        int tmpFontSize = 0;
        bool isNumber = true;
        const QStringList fontParts = parser.value("font").split(",");
        // Try to be user friendly, accept options given as...
        // 10 // Mono // Mono,10 // Mono,Bold // Mono,Bold,10 // Mono,10,Bold
        // We catch many bad settings, like no numbers as font/style or to much args
        for (const QString &part : fontParts) {
            if (part.toUInt(&isNumber) and ! tmpFontSize) {
                tmpFontSize = part.toUInt();
                continue;
            }
            if (fontFamily.isEmpty() and !isNumber) {
                fontFamily = part.trimmed();
                continue;
            }
            if (fontStyle.isEmpty() and !isNumber) {
                fontStyle = part.trimmed();
                continue;
            }
            qStdErr() << "Too much set: " << parser.value("font") << Qt::endl;
            return 1;
        }

        (tmpFontSize) ? fontSize = tmpFontSize : fontSize = MY_FONTSIZE;
        if (fontFamily.isEmpty()) fontFamily = MY_FONTFAMILY;
    }

    const QStringList marginStrList = parser.value("margins").split(",");
    for (const auto &marginVal : marginStrList) {
        qreal margin;
        if (marginVal.isEmpty()) {
            margin = defautMargin;
        } else {
            bool ok = true;
            margin = marginVal.toDouble(&ok);
            if (! ok) {
                qStdErr() << "Bad margin value: " << marginVal << Qt::endl;
                return 1;
            }
        }
        marginList << margin;
    }
    while (marginList.size() < 4) marginList << defautMargin;

    //
    // ...and continue to determine in/out files
    //

    const QStringList args = parser.positionalArguments();
    uint neededArguments = 1;
    if (parser.isSet("info")) {
        pdfFile = "[not yet set]";
        neededArguments = 0;
    }

    if (parser.isSet("test-page")) {
        txtFile = "[-> Test Page <-]";
        pdfFile = "/tmp/"  MY_NAME  "-test-page.pdf";
        // Now, that we do not need any user defined in/out file,
        // we make use of a taboo to avoid ugly nesting :-)
        goto ApplySettings;
    }

    if (parser.isSet("in-file")) {
        txtFile = parser.value("in-file");
        QFileInfo info(txtFile);
        if (! info.exists(txtFile)) {
            qStdErr() << "File not found: '" << txtFile << "" << Qt::endl;
            return 1;
        }
        txtFile = info.canonicalFilePath();
        txtFileName = info.fileName();
        neededArguments = 0;
        pdfFile = info.canonicalPath() + "/" + info.completeBaseName() + ".pdf";
    }

    if (args.size() < neededArguments) {
        parser.showHelp(1);
    }

    if (args.size() > 0) {
        pdfFile = args.at(0);
        QFileInfo info(pdfFile);
        if (! pdfFile.endsWith(".pdf")) {
            pdfFile = info.absolutePath() + "/" + info.completeBaseName() + ".pdf";
        }

        // Validate out file, yeah only if not implicit set by -i
        if (QFile::exists(pdfFile) and ! parser.isSet("force")) {
            qStdErr() << "File already exist: " << pdfFile << Qt::endl
                      << "Use --force if you don't care" << Qt::endl;
            return 1;
        }
    }

    if (txtFile.isEmpty() and args.size() == 2) {
        txtFile = args.at(1);
        QFileInfo info(txtFile);
        if (! info.exists()) {
            qStdErr() << "TXT file not found: " << txtFile << Qt::endl;
            return 1;
        }
        txtFile = info.canonicalFilePath();
        txtFileName = info.fileName();
    }

    //
    // We are close to finish, time to apply settings and poll the feedback
    // so we can calculate most important data: maxChar and maxLines
    //

ApplySettings: // Nasty goto label :-)

    QPrinter printer(QPrinter::HighResolution);
    if (! txtFileName.isEmpty()) printer.setDocName(txtFileName);
    printer.setCreator(app.applicationName() + " v" + app.applicationVersion());
    printer.setOutputFileName(pdfFile);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPageSize(pageSize);
    printer.setPageOrientation(pageOrientation);
    // printer.setFullPage(true); // NO! We want our margins

    printer.setPageMargins(QMarginsF(mmToPoints(marginList.at(LeftMargin))
                                   , mmToPoints(marginList.at(TopMargin))
                                   , mmToPoints(marginList.at(RightMargin))
                                   , mmToPoints(marginList.at(BottomMargin))));

    QFont font(QFont(fontFamily, fontSize), &printer);
    font.setStyleName(fontStyle);

    QFontMetricsF fm(font);
    // qMax(0, ...) to avoid negative values when user makes strange settings
    // Which char-width is "best/correct" I don't know, this one has worked in a couple of tests..
    int maxChar = qMax(0.0, printer.width() / fm.horizontalAdvance('X'));
    // ..while these fm.averageCharWidth() fm.maxWidth() sometimes differ, sometimes not, strange
    int maxLines = qMax(0.0, printer.height() / fm.height());

    if (parser.isSet("info") or parser.isSet("test-page")) {
        QFontInfo fi(font);
        qStdOut() << "Requested Font   : " << fontFamily << Qt::endl
                  << "Req Font Style   : " << fontStyle << Qt::endl
                  << "Req Font Size    : " << fontSize << Qt::endl
                  << "Used Font        : " << fi.family() << Qt::endl
                  << "Used Style       : " << fi.styleName() << Qt::endl
                  << "Used Size        : " << fi.pointSize() << Qt::endl
                  << "Has Fixed Pitch  : " << ((fi.fixedPitch()) ? "yes" : "NO") << Qt::endl
                  << "Page Size        : " << pageSize.name() << Qt::endl
                  << "Page Orientation : " << ((pageOrientation) ? "Landscape" : "Portrait") << Qt::endl
                  << "Max Lines        : " << maxLines << Qt::endl
                  << "Max Columns      : " << maxChar << Qt::endl
                  ;
        // Let's break another rule! This way looks the code nicer
        if (parser.isSet("info")) {
        qStdOut() << "In-File          : " << ((txtFile.isEmpty()) ? "<stdin>" : txtFile) << Qt::endl
                  << "Out-File         : " << pdfFile << Qt::endl; return 0; }
    }

    if (maxChar < 1 or maxLines < 1) {
            qStdErr() << "No print area" << Qt::endl;
            return 1;
    }

    // To collect the in-file
    QStringList content;

    if (parser.isSet("test-page")) {
        if (maxChar < 3 or maxLines < 2) {
            qStdErr() << "Note: Print area very limited, the test page may look strange or even bad" << Qt::endl;
        }
        // FIXME/TODO Perhaps is printing the margins useful(?)
        QString s = QString(" %1 char/line, %2 lines/page ").arg(maxChar).arg(maxLines);
        int gap = maxChar - s.size();
        QString f; // Filler
        QString h; // Help
        int i = 2; // Count lines
        if (gap < 6) {
            s = QString("< 1  %1 char/line").arg(maxChar);
            s.truncate(maxChar-2);
            f = " >";
            h = f.rightJustified(maxChar-s.size(), ' ');
            s = s + h;
            content << s;

            s = QString("  2  %1 lines/page").arg(maxLines);
            s.truncate(maxChar);
            content << s;
            i = 3;

            if (maxLines < 8 or maxChar < 20) {
                qStdErr() << "Note: Print area limited, skip font/option info" << Qt::endl;
                goto PrintLineNumbers;
            }

            // Let's break another rule! This way looks the code nicer
            if (parser.isSet("margins")) {
            s = QString("  %2  MO: %1").arg(parser.value("margins")).arg(i++);
            s.truncate(maxChar); content << s; }

            if (parser.isSet("font")) {
            s = QString("  %2  FO: %1").arg(parser.value("font")).arg(i++);
            s.truncate(maxChar); content << s; }

            QFontInfo fi(font);
            s = QString("  %2  Ft: %1").arg(fi.family()).arg(i++);
            s.truncate(maxChar); content << s;
            s = QString("  %2  St: %1").arg(fi.styleName()).arg(i++);
            s.truncate(maxChar); content << s;
            s = QString("  %2  Si: %1").arg(fi.pointSize()).arg(i++);
            s.truncate(maxChar); content << s;

            if (!fi.fixedPitch()) {
            s = QString("  %1  * NO FIXED PITCH *").arg(i++);
            s.truncate(maxChar); content << s;}

        } else {
            f = "< 1 ";
            h = f.leftJustified(gap/2, ' ');
            s = h + s;
            f = " >";
            h = f.rightJustified(maxChar-s.size(), ' ');
            s = s + h;
            content << s;

            if (maxLines < 8) {
                qStdErr() << "Note: Print area limited, skip font/option info" << Qt::endl;
                goto PrintLineNumbers;
            }

            // Let's break another rule! This way looks the code nicer
            if (parser.isSet("margins")) {
            s = QString("  %2  Margin Opt: %1").arg(parser.value("margins")).arg(i++);
            s.truncate(maxChar); content << s; }

            if (parser.isSet("font")) {
            s = QString("  %2  Font Opt  : %1").arg(parser.value("font")).arg(i++);
            s.truncate(maxChar); content << s; }

            QFontInfo fi(font);
            s = QString("  %2  Used Font : %1").arg(fi.family()).arg(i++);
            s.truncate(maxChar); content << s;
            s = QString("  %2  Used Style: %1").arg(fi.styleName()).arg(i++);
            s.truncate(maxChar); content << s;
            s = QString("  %2  Used Size : %1").arg(fi.pointSize()).arg(i++);
            s.truncate(maxChar); content << s;

            if (!fi.fixedPitch()) {
            s = QString("  %1  *** FONT HAS NO FIXED PITCH ***").arg(i++);
            s.truncate(maxChar); content << s;}
        }

PrintLineNumbers: // Once again nasty goto label

        for ( ; i < maxLines; ++i) {
            content << QString("  %1").arg(i);
        }

        if (i == maxLines) {
            f = QString("< %1").arg(maxLines);
            s = QString("last line >");
            s = s.right(maxChar - f.size());
            s = s.rightJustified(maxChar - f.size(), ' ');
            content << f+s;
        }

        // Well, we are slightly hasty with this statement. I guess Murphy is already grinning...
        qStdOut() << "Test page written to: " << pdfFile << Qt::endl;


    } else if (txtFile.isEmpty() and args.size() == 1) {
        QTextStream in(stdin);
        while(!in.atEnd()) {
            QString line = in.readLine();
            content.append(line);
        }
    } else {
        QFile file(txtFile);
        if(!file.open(QIODevice::ReadOnly)) {
            qStdErr() << file.errorString() << Qt::endl;
            return 1;
        }

        QTextStream in(&file);
        while(!in.atEnd()) {
            QString line = in.readLine();
            content.append(line);
        }

        file.close();
    }

    // Here is the beef! Create the PDF
    QTextDocument doc;
    doc.setDefaultFont(font);
    doc.setDocumentMargin(0.0);
    doc.setPageSize(QSizeF(printer.pageRect(QPrinter::DevicePixel).size()));
    doc.setPlainText(content.join("\n"));
    doc.print(&printer);
}

// That's all folks!
