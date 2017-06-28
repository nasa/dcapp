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
    
    @IBOutlet weak var startDisplayButton: UIButton!
    
    override func viewDidLoad()
    {
        super.viewDidLoad()
        
//         This causes the model controller to be instantiated
        _ = modelController
        
        glContext = EAGLContext( api: .openGLES1)
    }
    
    func TimerFunc()
    {
        Bridged_TimerUpdate()
        
        if modelController.doesNeedDisplay(indexA: currentDisplay )
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
                
                glkView.display()
            }
        }
    }
    
    override var prefersStatusBarHidden: Bool {
        return true
    }
    
    public func glkView(_ view: GLKView, drawIn rect: CGRect)
    {
        print( "rect= \(rect)");
        Bridged_Draw()
    }
    
    @IBAction func onStartDisplay(_ sender: Any)
    {
        //        startDisplayButton.isHidden = true
        //        let fileNameL : String = "animate.xml"

            
        UIView.animate(withDuration: 0.75, delay: 0.01, options: .curveEaseIn,
                       animations: { self.startDisplayButton.frame = self.startDisplayButton.frame.offsetBy(dx: 0, dy: 800); },
                       completion: { ( finshed : Bool ) in
//                        let fileNameL : String = "NexSys/mpcv.xml"
                        let fileNameL : String = "colors.xml"
                        
                        Bridged_Initialize( UnsafeMutablePointer(mutating: fileNameL) )
                        
                        self.modelController.markAllWindowsForRedraw()
        } )
        
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
