local remote_connection = require("remote/remote_connection")
local remote_file_utils = require("remote/remote_file_utils")
local remote_application_utils = require("remote/remote_application_utils")

ATF = {}

if config.remoteConnection.enabled then
  ATF.remoteConnection = remote_connection.RemoteConnection(config.remoteConnection.url, config.remoteConnection.port)
  ATF.remoteConnection:Connect()
  ATF.remoteUtils = {}
  ATF.remoteUtils.file = remote_file_utils.RemoteFileUtils(ATF.remoteConnection)
  ATF.remoteUtils.app = remote_application_utils.RemoteAppUtils(ATF.remoteConnection)
end

return ATF
