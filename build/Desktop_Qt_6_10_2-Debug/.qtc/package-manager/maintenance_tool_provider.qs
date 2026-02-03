accountFieldsVisible = function()
{
    var qtPackagesString = installer.environmentVariable("QTC_MAINTENANCE_TOOL_QT_PACKAGES").toString();
    qtPackagesString = qtPackagesString.split(";").join(" ");

    var result = QMessageBox.question("qtcreator.install.packages", "Qt Creator",
                                      "CMake could not find: " + qtPackagesString + "<br><br>" +
                                      "Do you want to install the missing packages?",
                                      QMessageBox.Yes | QMessageBox.No, QMessageBox.Yes);
    if (result == QMessageBox.No) {
        gui.rejectWithoutPrompt();
    } else {
        var ifwVersion = installer.value("FrameworkVersion");
        if (installer.versionMatches(ifwVersion, "=4.9.0"))
            gui.clickButton("submitButtonLogin");
        else
            gui.clickButton(buttons.NextButton);
    }
}

usageStatisticVisible = function()
{
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.CredentialsPageCallback = function()
{
    var page = gui.currentPageWidget();
    page.accountFieldsVisible.connect(accountFieldsVisible)
    var ifwVersion = installer.value("FrameworkVersion");
    if (installer.versionMatches(ifwVersion, "=4.9.0"))
        page.usageStatisticVisible.connect(usageStatisticVisible);
}

Controller.prototype.IntroductionPageCallback = function()
{
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.ComponentSelectionPageCallback = function()
{
    var componentsString = installer.environmentVariable("QTC_MAINTENANCE_TOOL_COMPONENTS");
    var componentsList = componentsString.split(";");
    for (var idx = 0; idx < componentsList.length; idx++) {
        installer.selectComponent(componentsList[idx]);
    }
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.FinishedPageCallback = function()
{
    gui.clickButton(buttons.FinishButton);
}

function Controller()
{
    installer.installationFinished.connect(function() {
        gui.clickButton(buttons.NextButton);
    })
}
