//
//  MainViewController.swift
//  DCApp_iOS
//
//  Created by Shores, David L. (JSC-ER711) on 7/3/17.
//  Copyright © 2017 Shores, David L. (JSC-ER711). All rights reserved.
//

import Foundation
import UIKit
import GLKit


class DCAppDisplay : GLKView
{
    var controller : ModelController? = nil
    
    override init(frame: CGRect, context: EAGLContext) {
        super.init(frame: frame, context: context )
    }
    
    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    var currentTouch : CGPoint = CGPoint(x: 0.0, y: 0.0)
    
    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?)
    {
        if let myTouches = event?.touches(for: self )
        {
            if let touch = myTouches.first
            {
                currentTouch = touch.location(in: self )
                currentTouch.y = self.frame.height - currentTouch.y

                Bridged_Input( Float(currentTouch.x), Float(currentTouch.y), 0 ); // 0 is pushed
            }
        }
    }
    
    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?)
    {
        Bridged_Input( Float(currentTouch.x), Float(currentTouch.y), 1 ); // 1 is released
    }
}

class MainViewController : UIViewController, GLKViewDelegate, ModelControllerDelegate
{
    var _modelController: ModelController? = nil
    var glkView : DCAppDisplay! = nil
    var glContext   : EAGLContext! = nil
    var myTimer : Timer! = nil
    var currentDisplay  : Int16 = -1
    
	@IBOutlet weak var dougHost: UITextField!
	@IBOutlet weak var dougPort: UITextField!
    @IBOutlet weak var startDisplayButton: UIButton!
	@IBOutlet weak var pixelStreamHost: UITextField!
	@IBOutlet weak var pixelStreamPort: UITextField!
	@IBOutlet weak var trickHost: UITextField!
	@IBOutlet weak var trickPort: UITextField!
	@IBOutlet weak var dcAppHost: UITextField!
	@IBOutlet weak var dcAppPort: UITextField!
	@IBOutlet weak var editFieldSys: UIStackView!
	@IBOutlet weak var forceDownload: UISwitch!
    
    override func viewDidLoad()
    {
        super.viewDidLoad()
        
//         This causes the model controller to be instantiated
        _ = modelController
        
        glContext = EAGLContext( api: .openGLES1)
		
		dougHost.text			= "192.168.1.5"
		dougPort.text			= "5451"
		pixelStreamHost.text	= "192.168.1.5"
		pixelStreamPort.text	= "8080"
		trickHost.text			= "192.168.1.5"
		trickPort.text			= "56056"
		dcAppHost.text			= "192.168.1.5"
		dcAppPort.text			= "8042"
    }
    
    func TimerFunc()
    {
        Bridged_TimerUpdate()
        
        if modelController.doesNeedDisplayNoClear(indexA: currentDisplay )
        {
            glkView.display()
        }
    }
    
    func newViewAvailable( indexA : Int16 )
    {
        if glkView == nil
        {
            if let windowData = modelController.getWindowData(indexA: indexA )
            {
                currentDisplay = indexA
                
                let xL = Int(self.view.frame.width / 2) - Int(windowData.width/2)
                let yL = Int(self.view.frame.height / 2) - Int(windowData.height/2)
                let rect = CGRect(
                    origin: CGPoint(x: xL, y: yL),
                    size: CGSize(width: Int(windowData.width), height: Int(windowData.height) )
                )
                
                glkView = DCAppDisplay.init(frame: rect, context: glContext )
                
                self.view.addSubview( glkView )
                
                glkView.delegate = self
                glkView.drawableDepthFormat = .format16
                glkView.drawableMultisample = .multisampleNone
                
                glkView.controller = modelController
                
                glkView.enableSetNeedsDisplay = false
                
                glkView.bindDrawable()
                
                myTimer = Timer.scheduledTimer(timeInterval: 0.01, target: self, selector: #selector( TimerFunc ), userInfo: nil, repeats: true )
                
//                glkView.display()
            }
        }
    }
	
	@IBAction func ToggleSameHost(_ sender: UISwitch)
	{
		if sender.isOn
		{
//			dougHost
			
			pixelStreamHost.text	= dougHost.text
			trickHost.text			= dougHost.text
			dcAppHost.text			= dougHost.text
			
			pixelStreamHost.isEnabled = false
			pixelStreamHost.textColor	= UIColor.lightGray
			trickHost.isEnabled = false
			trickHost.textColor	= UIColor.lightGray
			dcAppHost.isEnabled = false
			dcAppHost.textColor	= UIColor.lightGray
		}
		else
		{
			pixelStreamHost.isEnabled = true
			trickHost.isEnabled = true
			dcAppHost.isEnabled = true
			
			pixelStreamHost.textColor	= UIColor.black
			trickHost.textColor			= UIColor.black
			dcAppHost.textColor			= UIColor.black
		}
	}
	@IBAction func ToggleForceLoad(_ sender: UISwitch)
	{
		
	}
	
    override var prefersStatusBarHidden: Bool {
        return true
    }
    
    public func glkView(_ view: GLKView, drawIn rect: CGRect)
    {
//        print( "rect= \(rect)");
        Bridged_Draw()
    }
    
    @IBAction func onStartDisplay(_ sender: Any)
    {
		if let dougHostName = dougHost.text,
			let dougPortNumber = dougPort.text,
			let psHostName = pixelStreamHost.text,
			let psHostPort = pixelStreamPort.text,
			let trickHostName = trickHost.text,
			let trickHostPort = trickPort.text,
			let dcAppHostName = dcAppHost.text,
			let dcAppHostPort = dcAppPort.text,
			let forceDownload = forceDownload
		{
			Bridged_SetDougHostAndPort( UnsafeMutablePointer(mutating: dougHostName), UnsafeMutablePointer(mutating: dougPortNumber) )
			Bridged_SetPixelStreamHostAndPort( UnsafeMutablePointer(mutating: psHostName ), UnsafeMutablePointer(mutating: psHostPort ) )
			Bridged_SetTrickHostAndPort( UnsafeMutablePointer(mutating: trickHostName ), UnsafeMutablePointer(mutating: trickHostPort ) )
			Bridged_SetdcAppHostAndPort( UnsafeMutablePointer(mutating: dcAppHostName ), UnsafeMutablePointer(mutating: dcAppHostPort ) )
			Bridged_SetForceDownload( forceDownload.isOn ? 1 : 0 )
			
			UIView.animate(withDuration: 0.75, delay: 0.01, options: .curveEaseIn,
						   animations: { self.startDisplayButton.frame = self.startDisplayButton.frame.offsetBy(dx: 0, dy: 800); },
						   completion: { ( finshed : Bool ) in
							
							self.startDisplayButton.isHidden = true
							self.editFieldSys.isHidden = true
							
							Bridged_Initialize( UnsafeMutablePointer(mutating: dcAppHostName ), UnsafeMutablePointer(mutating: dcAppHostPort ) )
							
							self.modelController.markAllWindowsForRedraw()
			} )
			
		}
		
    }
    
    var modelController: ModelController {
        // Return the model controller object, creating it if necessary.
        // In more complex implementations, the model controller may be passed to the view controller.
        if _modelController == nil {
            _modelController = ModelController()
            _modelController?.delegate = self
            
            if let app = UIApplication.shared.delegate as! AppDelegate!
            {
                app.myModelController = _modelController
            }
        }
        return _modelController!
    }
}
